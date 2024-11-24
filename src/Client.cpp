#include "../include/Client1.hpp"

const std::string Client::delim = "\r\n\r\n";
const std::map<int, std::string> http_statuses(initHttpStatuses());
const char *Client::content_length = "Content-Length", *Client::transfer_encoding = "Transfer-Encoding";

std::map<int, std::string> Client::initHttpStatuses() const {
    std::map<int, std::string> http_statuses;

    http_statuses[200] = "OK";
    http_statuses[201] = "Created";
    http_statuses[400] = "Bad Request";
    http_statuses[403] = "Forbidden";
    http_statuses[404] = "Not Found";
    http_statuses[405] = "Method Not Allowed";
    return http_statuses;
}

Client::Client(int fd, std::vector<ServerInfo> const& servers)
        : state(START_LINE),
        sockfd(fd),
        http_status(200),
        route(NULL),
        server(initServer(servers)),
        recv_offset(0),
        send_offset(0),
        track_length(false),
        unchunk_flag(false),
        bytes_left(0)
{
    assert(server);
}

void Client::socketRecv() {
    int bytes;
    char buf[BUFSIZE + 1];

    assert(state != ERROR);
    bytes = recv(sockfd, buf, BUFSIZE, MSG_DONTWAIT);
    if (bytes <= 0) {
        return setInvalidState();
    }
    parseHttpRequest(buf, bytes);
}

void Client::socketSend() {
    int bytes;

    assert(state == ERROR || state == SEND_READY);
    bytes = send(sockfd, sendbuf.c_str() + send_offset, sendbuf.length() - send_offset, MSG_DONTWAIT);
    if (bytes <= 0) {
        return setInvalidState();
    }
    if (bytes != sendbuf.length() - send_offset) {
        send_offset += bytes;
    } else {
        setFinishedState();
    }
}

void Client::setState(int state) {

    this->state = state;
}

int Client::getState() const {

    return state;
}

RouteInfo const* Client::findRoute() const {
    size_t findpos, len, max_len;
    std::vector<RouteInfo>::const_iterator it, ite;

    if (server.routes.empty()) {
        return NULL;
    }
    max_len = 0;
    it = ite = server.routes.end();
    for (std::vector<RouteInfo>::const_iterator p = server.routes.begin(); p != ite; ++p) {
        len = p->prefix_str.length();
        findpos = request_uri.find(p->prefix_str);
        if (findpos != std::string::npos && len > max_len) {
            it = p;
            max_len = len;
        }
    }
    assert(it != ite);
    return &*it;
}

int Client::parseStartLine() {
    std::istringstream iss;
    std::string http_str, http_version;

    pos = recvbuf.find("\r\n");
    if (pos == std::string::npos) {
        return NEED_MORE;
    }
    iss.str(recvbuf.substr(0, pos));
    if (!(iss >> http_str) || !(iss >> request_uri) || !(iss >> http_version)) {
        setErrorState(1);
    }
    if (http_str != "GET") {
        http_method = GET_METHOD;
    } else if (http_str != "POST") {
        http_method = POST_METHOD;
    } else if (http_str != "DELETE") {
        http_method = DELETE_METHOD;
    } else {
        return setInvalidState(400);
    }

    route = findRoute();
    if (route) {
        // Directory where file should be searched from
        // PATH_INFO could be in uri, eg /infile in .../script.cgi/infile
        request_uri.replace(0, request_uri.find(route->prefix_str), route->root);
    }
                                           //
    if (http_version != "HTTP/1.1") {
        setErrorState(4); // arbitrary numbers for now
    }
    recvbuf.erase(0, pos + 2);
    setState(HEADERS);
    return !NEED_MORE;
}

int Client::parseHeaders() {
    size_t pos;
    std::string buf;
    std::stringstream oss;

    pos = recvbuf.find(delim);
    if (pos == std::string::npos) {
        return NEED_MORE;
    }
    oss.str(recvbuf.substr(0, pos));
    while (!getline(oss, buf).eof()) {
        if (!oss) {
            oss.exceptions(oss.rdstate());
        } else {
            pos = str.find(':');
            assert(pos != std::string::npos);
            headers[buf.substr(0, pos)] = buf.substr(pos + 2);
        }
    }
    if (http_method == POST_METHOD) {
        recvbuf.erase(0, pos + delim.length());
        setState(MSG_BODY);
    } else {
        setState(RECV_DONE);
    }
    return !NEED_MORE;
}

int Client::unchunkRequest() {
    uint64_t bytes;
    std::istringstream iss;
    std::string::const_iterator it1, it2, ite;
    std::string::const_reverse_iterator r_it1, r_it2, r_ite;
    const char *delim_str = delim.c_str();
    const size_t delim_length = delim.length();

    it1 = it2 = recvbuf.begin();
    ite = recvbuf.end();
    r_ite = recvbuf.rend();
    while (1) {
        it1 = std::find(it1, ite, '\r');
        if (it1 == ite || std::distance(it1, ite) < delim_length) {
            break ;
        }
        if (!strncmp(&*it1, delim_str, delim_length)) { // check if its "\r\n\r\n"
            r_it1 = it1;
            r_it2 = std::find_if(++r_it1, r_ite, std::binary_negate<int(*)(int)>(&isdigit));
            iss.str(std::string(r_it1, r_it2));
            if (!(iss >> bytes)) { // integer overflow
                return setInvalidState();
            }
            iss.clear();
            if (!bytes) {
                return setState(RECV_DONE), !NEED_MORE;
            }
            std::advance(it1, delim_length);
            if (std::distance(it1, ite) < bytes) {
                break ; // need more bytes, wait for next recv
            }
            std::copy(it1, it1 + bytes, std::back_inserter(msg_body));
            it1 = std::find(it1, ite, '\n');
            it2 = it1 + 1; // only after a successful extraction, then we move the it2(old) iterator forward
        } else {
            ++it1;
        }
    }
    recvbuf.erase(recvbuf.begin(), it2);
    return NEED_MORE;
}

int Client::trackRecvBytes() {
    size_t pos;
    size_t bytes;
    std::string::const_iterator it, ite = recvbuf.end();

    bytes = std::min(bytes_left, recvbuf.length());
    std::copy(recvbuf.begin(), recvbuf.begin() + bytes, std::back_inserter(msg_body));
    bytes_left -= bytes;
    if (!bytes_left) {
        return setState(RECV_DONE), !NEED_MORE;
    } else {
        return recvbuf.clear(), NEED_MORE;
    }
}

int Client::parseMsgBody() { // need to handle chunked encoding
    static bool config_flag = false;

    if (!config_flag) {
        if (headers.count(content_length) && headers.count(transfer_encoding)) {
            setInvalidState(); // both headers should never exist together
        }
        if (headers.count(content_length)) {
            track_length = true;
            bytes_left = atoi(headers[content_length].c_str());
        }
        if (headers.count(transfer_encoding) && headers[transfer_encoding] == "chunked") {
            unchunk_request = true;
        }
        config_flag = true;
    }
    // Content-Length and Transfer-Encoding should never be included together
    // only insert the needed info into msg_body vector
    if (track_length) {
        return trackRecvBytes();
    } else if (unchunk_flag) {
        return unchunkRequest();
    } else {
        std::copy(recvbuf.begin(), recvbuf.end(), std::back_inserter(msg_body));
        return NEED_MORE; // only stops when eof is detected during recv call
    }
}

void Client::parseHttpRequest(const char *buf, int bytes) {
    int res;

    recvbuf.append(buf, bytes);
    do {
        switch (state) {
            case START_LINE:        res = parseStartLine(), continue ;
            case HEADERS:           res = parseHeaders(), continue ;
            case MSG_BODY:          res = parseMsgBody(), continue ;
            case RECV_DONE:         fillSendBuf(), break ; // may not be able to get everything immediately
            case ERROR:             fillErrorState(), break ;
            default:                throw std::exception(); // should not reach here
        }
    }
    while (res != NEED_MORE);
}

void Client::setStartLine(int code) {
    std::ostringstream oss;

    oss << "HTTP/1.1 " << code << ' ' << Client::http_statuses[code] << "\r\n";
    sendbuf = oss.str();
}

int Client::getRequestedFile(std::string const& filename) {
    std::ifstream infile(filename);

    if (!infile.is_open()) {
        return ERROR;
    }
    std::copy((std::istreambuf_iterator<char>(infile)),
        std::istreambuf_iterator<char>(), std::back_inserter(sendbuf));
    return !ERROR;
}

void Client::fillSendBuf(int code) { // input can come from cgi script
    size_t pos;
    std::string pathinfo;
    std::ifstream infile;
    std::ostringstream oss;
    std::string::iterator start, it1, it2, ite;
    std::map<std::string, std::string> query_str;

    oss << "HTTP/1.1 " << code << ' ' << Client::http_statuses[code] << "\r\n";
    oss << "Content-Type: " << getContentType() << "\r\n\r\n";
    sendbuf = oss.str();
    if (state == ERROR) {
        ;
        return ;
    }
    ite = request_uri.end();
    it1 = start = std::find(request_uri.begin(), ite, '?');
    while (it1 != ite) {
        it1 = std::find(it1, ite, '=');
        if (it1 == ite) {
            break ;
        }
        it2 = std::find(it1 + 1, ite, '&');
        query_str[std::string(start, it1)] = std::string(it1 + 1, it2);
        it1 = it2;
    }
    if (start != ite) {
        *start = '\0';
    }
    if (route && !route.cgi_extension.empty()
        && (pos = request_uri.find(route.cgi_extension)) != std::string::npos) {
        pathinfo = request_uri.substr(pos + route.cgi_extension.length() + 1);
        // may need to extract out QUERY_STRING and other stuff from url
        // prepare environment vars for cgi script
        request_uri[pos] = '\0';
    } else {
        infile.open(request_uri);
        if (!infile.is_open()) {

        }
    }
}

void Client::setFinishedState() {

    epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, NULL);
    close(sockfd);
    state = FINISHED;
}

int Client::setInvalidState(int err) {

    setState(ERROR);
    setStartLine(err);
    setHeaders();
    return ERROR;
}

ServerInfo const& Client::initServer(std::vector<ServerInfo> const& servers) const {
    std::vector<ServerInfo>::const_iterator ite = servers.end();

    for (std::vector<ServerInfo>::const_iterator it = servers.begin(); it != ite; ++it) {
        if (it->sockfd == sockfd) {
            return *it;
        }
    }
    return assert(false), *ite; // should never reach here
}
