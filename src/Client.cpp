#include "../include/Client1.hpp"

int Client::epollfd = 0;
char **Client::envp = NULL;
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
        active_fd(fd),
        passive_fd(-1),
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

bool Client::operator==(int sockfd) const {

    return sockfd == active_fd;
}

void Client::socketRecv() {
    ssize_t bytes;
    char buf[BUFSIZE + 1];

    assert(state != ERROR);
    bytes = recv(active_fd, buf, BUFSIZE, MSG_DONTWAIT);
    if (bytes <= 0) {
        return setInvalidState();
    }
    switch (io_state) {
        case RECV_HTTP:     parseHttpRequest(buf, bytes), break ;
        case RECV_CGI:      parseCgiOutput(buf, bytes), break ; // both parse funcs may change IOState
        default:            std::terminate();
    }
}

void Client::socketSend() {
    ssize_t bytes;

    assert(io_state == SEND_HTTP || io_state == SEND_CGI);
    bytes = send(active_fd, &*send_it, std::distance(send_it, send_ite), MSG_DONTWAIT);
    switch (bytes) {
        case -1:        handle_error("send");
        case 0:         closeConnection(), break ;
    }
    if (bytes <= 0) {
        return setInvalidState();
    }
    send_it += bytes;
    if (send_it == send_ite) {
        setFinishedState();
    }
}

void Client::setIOState(int state) {

    this->io_state = io_state;
}

int Client::getIOState() const {

    return io_state;
}

void Client::setPState(int state) {

    this->state = state;
}

int Client::getPState() const {

    return p_state;
}

void Client::setEnvp(char **envp) {

    Client::envp = envp;
}

void Client::setEpollfd(int epollfd) {

    Client::epollfd = epollfd;
}

int Client::getEpollfd() const {

    return Client::epollfd;
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

bool Client::parseStartLine(const char *buf, size_t bytes) {
    std::istringstream iss;
    std::string http_str, http_version;

    pos = recvbuf.append(buf, bytes).find("\r\n");
    if (pos == std::string::npos) {
        return false;
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
    return true;
}

bool Client::parseHeaders(const char *buf, size_t bytes) {
    size_t pos;
    std::string buf;
    std::stringstream oss;

    pos = recvbuf.append(buf, bytes).find(delim);
    if (pos == std::string::npos) {
        return false;
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
        msgbody = recvbuf.erase(0, pos + delim.length());
        setState(MSG_BODY);
    } else {
        setState(FINISHED);
    }
    return true;
}

bool Client::unchunkRequest() { // only unchunking of request will require a intermediate temp buffer to selectively read bytes into msgbody
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
                setPState(FINISHED)
                return true;
            }
            std::advance(it1, delim_length);
            if (std::distance(it1, ite) < bytes) {
                break ; // need more bytes, wait for next recv
            }
            msgbody.insert(msg_body.end(), it1, it1 + bytes);
            it1 = std::find(it1, ite, '\n');
            it2 = it1 + 1; // only after a successful extraction, then we move the it2(old) iterator forward
        } else {
            ++it1;
        }
    }
    recvbuf.erase(recvbuf.begin(), it2);
    return false;
}

bool Client::trackRecvBytes(const char *buf, size_t bytes) {
    size_t bytes;

    bytes = std::min(bytes_left, bytes);
    msgbody.append(buf, bytes);
    bytes_left -= bytes;
    if (!bytes_left) {
        setState(FINISHED)
        return true;
    }
    return false;
}

void Client::configureIOMethod() {

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
}

bool Client::parseMsgBody(const char *buf, size_t bytes) { // need to handle chunked encoding

    if (!config_flag) {
        configureIOMethod();
        config_flag = true;
    }
    // Content-Length and Transfer-Encoding should never be included together
    // only insert the needed info into msg_body vector
    if (track_length) {
        return trackRecvBytes(buf, bytes);
    } else if (unchunk_flag) {
        return unchunkRequest(buf, bytes);
    } else {
        msgbody.append(buf, bytes);
        return false; // only stops when eof is detected during recv call
    }
}

void Client::parseHttpRequest(const char *buf, size_t bytes) {
    bool res;

    do {
        switch (p_state) {
            case START_LINE:        res = parseStartLine(buf, bytes), continue ;
            case HEADERS:           res = parseHeaders(buf, bytes), continue ;
            case MSG_BODY:          res = parseMsgBody(buf, bytes), continue ; // IF GET REQUEST, SHOULDNT ENTER HERE
            case ERROR:
            case FINISHED:          res = prepareResource(), continue ;
            default:                throw std::exception(); // should not reach here
        }
    }
    while (res);
}

void Client::writeInitialPortion(int httpcode) {
    std::ostringstream oss;

    assert(Client::http_statuses.count(httpcode));
    oss << "HTTP/1.1 " << httpcode << ' ' << Client::http_statuses[httpcode] << "\r\n";
    oss << "Content-Type: " << getContentType() << "\r\n\r\n";
    sendbuf = oss.str();
}

bool Client::prepareResource() { // input can come from cgi script
    size_t pos;
    int pipefds[2];
    std::string pathinfo;
    std::ifstream infile;
    std::string::iterator start, it1, it2, ite;
    std::map<std::string, std::string> querystr;

    writeInitialPortion();
    ite = request_uri.end();
    it1 = start = std::find(request_uri.begin(), ite, '?');
    while (it1 != ite) {
        it1 = std::find(it1, ite, '=');
        if (it1 == ite) {
            break ;
        }
        it2 = std::find(it1 + 1, ite, '&');
        queryStr[std::string(start, it1)] = std::string(it1 + 1, it2);
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
        if (socketpair(AF_LOCAL, SOCK_STREAM, 0, pipefds) == -1) {
            handle_error("socketpair");
        }
        switch (fork()) {
            case -1:        handle_error("fork");
            case 0:         close(pipefds[1]), executeCgi(pipefds[0], pathinfo);
            default:        close(pipefds[0]), registerEvent(pipefds[1]); // server needs to send data to cgi script, then read data from script afterwards
                                                                          // state has to be set to either RECV or SEND
        }
        std::swap(active_fd, passive_fd = pipefds[1]);
        // assuming its a POST request to CGI script
        assert(http_method == POST_METHOD);
        send_it = msgbody.begin();
        send_ite = msgbody.end();
        setIOState(SEND_CGI);

    } else if (writeFileToSendbuf(request_uri) != ERROR) { // only if its non-cgi GET_REQUEST
        send_it = sendbuf.begin();
        send_ite = sendbuf.end();
        setIOState(SEND_HTTP);
    } else {
        return true;
    }
    return false;
}

void Client::registerEvent(int fd) {
    struct epoll_event ev;

    memset(&ev, 0, sizeof(ev));
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLOUT;
    if (epoll_ctl(Client::epollfd, EPOLL_CTL_ADD, fd, ev) == -1) {
        handle_error("RE: epoll_ctl");
    }
}

void Client::executeCgi(int pipefd, std::string const& pathinfo) {
    std::vector<char *> argv;
    std::vector<char *> envp;

    if (dup2(pipefd, STDIN_FILENO) == -1 || dup2(pipefd, STDOUT_FILENO) == -1) {
        handle_error("dup2");
    }
    if (close(pipefd) == -1) {
        handle_error("close");
    }
    argv.push_back(std::strdup(request_uri.c_str()));
    argv.push_back(std::strdup(pathinfo.c_str()));
    argv.push_back(NULL);
    for (int i = 0; Client::envp[i] != NULL; ++i) {
        envp.push_back(std::strdup(envp[i]));
    }
    // need to provide more env vars to cgi script based on rfc spec
    envp.push_back(NULL);
    execve(argv[0], argv.data(), envp.data());
    perror(argv[0]);
    std::for_each(argv.begin(), argv.end(), &free);
    std::for_each(envp.begin(), envp.end(), &free);
    // may need to close all other open fds to prevent fd leak
    exit(EXIT_FAILURE); // may cause memory leak
}

void Client::setFinishedState() {

    epoll_ctl(Client::epollfd, EPOLL_CTL_DEL, active_fd, NULL);
    close(active_fd);
    state = FINISHED;
}

int Client::writeFileToSendbuf(std::string const& filename) {
    std::ifstream infile;
    const char *filestr;
    
    filestr = filename.c_str();
    if (access(filestr, F_OK) == -1) {
        return setErrorState(404);
    }
    infile.open(filestr);
    if (!infile.is_open()) {
        return setErrorState(403);
    }
    std::copy((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>(), std::back_inserter(sendbuf));
    return !ERROR;
}

void Client::setErrorState(int num) {

    setState(ERROR);
    httpcode = num;
    if (ServerInfo::error_pages.count(http_code)) {
        request_uri = ServerInfo::error_pages[http_code];
        assert(!access(request_uri, F_OK | R_OK));
    } else {
        request_uri.clear();
    }
}

ServerInfo const& Client::initServer(std::vector<ServerInfo> const& servers) const {
    std::vector<ServerInfo>::const_iterator it, ite;

    ite = servers.end();
    for (it = servers.begin(); it != ite; ++it) {
        if (it->sockfd == active_fd) {
            break ;
        }
    }
    assert(it != ite); // should never reach end
    return *it;
}

Client::Client() {}

Client::~Client() {

    if (epoll_ctl(Client::epoll_fd, EPOLL_CTL_DEL, active_fd, NULL) == -1) {
        handle_error("dtor: epoll_ctl 1");
    }
    close(active_fd);
    if (passive_fd != -1) {
        if (epoll_ctl(Client::epoll_fd, EPOLL_CTL_DEL, passive_fd, NULL) == -1) {
            handle_error("dtor: epoll_ctl 2");
        }
        close(passive_fd);
    }
}

Client::Client(Client const&) {}

Client& Client::operator=(Client const&) { return *this; }
