#include "Client.hpp"

void Client::parseHttpRequest(const char *buf, size_t bytes) {
    int res;

    assert(p_state != ERROR);
    if (p_state == MSG_BODY && !unchunk_flag) {
        if (track_length) {
            bytes = std::min(bytes_left, bytes);
        }
        msg_body.append(buf, bytes);
    } else {
        recvbuf.append(buf, bytes);
    }
    do {
        switch (p_state) {
            case START_LINE:        res = parseStartLine(); continue ;
            case HEADERS:           res = parseHeaders(bytes); continue ;
            case MSG_BODY:          res = parseMsgBody(bytes); continue ; // DONT ENTER IF NON-POST REQUEST
            case FINISHED:          res = performRequest(); continue ;
            case ERROR:             setPState(START_LINE); return ;
            default:                std::terminate(); // should not reach here
        }
    }
    while (res);
}

RouteInfo const* Client::findRouteInfo() const {
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
    size_t pos;
    int http_method = 0;
    std::istringstream iss;
    std::string http_str, http_version;
    static const std::string newline = "\r\n";
    static const std::string methods[] = { "GET", "POST", "DELETE" };

    pos = recvbuf.find(newline);
    if (pos == std::string::npos) {
        return 0;
    }
    iss.str(recvbuf.substr(0, pos));
    if (!(iss >> http_str) || !(iss >> request_uri) || !(iss >> http_version)) {
        return setErrorState(1), 1;
    }
    for (int i = 0; i < 3; ++i) {
        if (http_str == methods[i]) {
            http_method = 1 << i;  // 1, 2, 4 for GET, POST, DELETE
            break ;
        }
    }
    assert(recvbuf.length() >= pos + newline.length());
    recvbuf.erase(0, pos + newline.length());
    if (!http_method) {
        setErrorState(404);
    } else if (http_version != "HTTP/1.1") {
        setErrorState(4); // arbitrary numbers for now
    } else {
        route = findRouteInfo();
        assert(route);
        // Directory where file should be searched from
        // PATH_INFO could be in uri, eg /infile in .../script.cgi/infile
        this->http_method = http_method;
        request_uri.replace(0, request_uri.find(route->prefix_str), route->root);
        setPState(HEADERS);
    }
    return 1;
}

int Client::parseHeaders(size_t& bytes) {
    size_t pos, pos1;
    std::string buf;
    std::istringstream iss;
    static const std::string delim = "\r\n\r\n";

    pos = recvbuf.find(delim);
    if (pos == std::string::npos) {
        return 0;
    }
    iss.str(recvbuf.substr(0, pos));
    while (!getline(iss, buf).eof()) {
        if (!iss) {
            iss.exceptions(iss.rdstate());
        } else {
            pos1 = buf.find(':');
            if (pos1 != std::string::npos) {
                headers[buf.substr(0, pos1)] = buf.substr(pos1 + 2, buf.length() - pos1 - 3); // + 2 for "\r\n"
            }
        }
    }
    //std::copy(headers.begin(), headers.end(), std::ostream_iterator<std::map<std::string, std::string>::value_type>(std::cout, "\n"));
    if (!configureIOMethod(headers)) {
        return -1;
    }
    if (http_method == POST_METHOD) {
        if (unchunk_flag) {
            recvbuf.erase(0, pos + delim.length());
        } else {
            msg_body.append(recvbuf.begin() + delim.length() + pos, recvbuf.end());
            recvbuf.clear();
            if (track_length) {
                assert(bytes_left >= msg_body.length());
                bytes = msg_body.length();
            }
        }
        setPState(MSG_BODY);
    } else {
        setPState(FINISHED);
    }
    return 1;
}

int Client::parseMsgBody(size_t bytes) { // need to handle chunked encoding

    if (track_length) {
        return trackRecvBytes(bytes);
    } else if (unchunk_flag) {
        return unchunkRequest();
    } else {
        return 0; // only stops when eof is detected during recv call
    }
}

bool Client::configureIOMethod(std::map<std::string, std::string> const& headers) {
    static const std::string content_length = "Content-Length", transfer_encoding = "Transfer-Encoding";

    if (headers.count(content_length) && headers.count(transfer_encoding)) {
        setErrorState(1); // both headers should never exist together
        return false;
    }
    if (headers.count(content_length)) {
        track_length = true;
        if (!(std::istringstream(headers.find(content_length)->second) >> bytes_left)) { // num too large
            setErrorState(1);
            return false;
        }
    }
    if (headers.count(transfer_encoding) && headers.find(transfer_encoding)->second == "chunked") {
        unchunk_flag = true;
    }
    return true;
}

int Client::trackRecvBytes(size_t bytes) {

    assert(bytes <= bytes_left);
    bytes_left -= bytes;
    if (!bytes_left) {
        return setPState(FINISHED), 1;
    } else {
        return 0;
    }
}

int Client::unchunkRequest() { // only unchunking of request will require a intermediate temp buffer to selectively read bytes into msg_body
    uint64_t bytes;
    std::istringstream iss;
    std::string::const_iterator it1, it2, ite;
    std::string::const_reverse_iterator r_it1, r_it2, r_ite;
    static const std::string newline = "\r\n";

    it1 = it2 = recvbuf.begin();
    ite = recvbuf.end();
    r_ite = recvbuf.rend();
    while (1) {
        it1 = std::find(it1, ite, '\r');
        if (it1 == ite || std::distance(it1, ite) < static_cast<std::ptrdiff_t>(newline.length())) {
            break ;
        }
        if (!strncmp(&*it1, newline.c_str(), newline.length())) { // check if its "\r\n"
            r_it1 = recvbuf.rend() - std::distance(static_cast<std::string::const_iterator>(recvbuf.begin()), it1) - 1;
            //r_it2 = std::find_if(++r_it1, r_ite, std::unary_negate<std::pointer_to_unary_function<int, int> >(std::ptr_fun(&isdigit)));
            r_it2 = std::find_if(++r_it1, r_ite, std::not1(std::ptr_fun(&isdigit)));
            iss.str(std::string(r_it1, r_it2));
            if (!(iss >> bytes)) { // integer overflow
                return setErrorState(8), -1;
            }
            iss.clear();
            if (!bytes) {
                return setPState(FINISHED), 1;
            }
            std::advance(it1, newline.length());
            if (std::distance(it1, ite) < static_cast<std::ptrdiff_t>(bytes)) {
                break ; // need more bytes, wait for next recv
            }
            msg_body.insert(msg_body.end(), it1, it1 + bytes);
            it1 = std::find(it1, ite, '\n');
            it2 = it1 + 1; // only after a successful extraction, then we move the it2(old) iterator forward
        } else {
            ++it1;
        }
    }
    recvbuf.erase(recvbuf.begin(), recvbuf.end() - std::distance(static_cast<std::string::const_iterator>(recvbuf.begin()), it2));
    return 0;
}
