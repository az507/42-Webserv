#include "Client.hpp"

void Client::parseHttpRequest(const char *buf, size_t bytes) {
    bool res;

    do {
        switch (p_state) {
            case START_LINE:        res = parseStartLine(buf, bytes), continue ;
            case HEADERS:           res = parseHeaders(buf, bytes), continue ;
            case MSG_BODY:          res = parseMsgBody(buf, bytes), continue ; //GET REQUEST==DONT ENTER HERE
            case FINISHED:          res = prepareResource(), continue ;
            case ERROR:             break ;
            default:                throw std::exception(); // should not reach here
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

bool Client::parseStartLine(const char *buf, size_t bytes) {
    size_t pos;
    std::istringstream iss;
    std::string http_str, http_version;
    static const std::string methods[] = { "GET", "POST", "DELETE" };

    pos = recvbuf.append(buf, bytes).find("\r\n");
    if (pos == std::string::npos) {
        return false;
    }
    iss.str(recvbuf.substr(0, pos));
    if (!(iss >> http_str) || !(iss >> request_uri) || !(iss >> http_version)) {
        setErrorState(1);
    }
    for (int i = 0; i < 3; ++i) {
        if (http_str == methods[i]) {
            http_method = !i ? 1 : i << 1, break ;
        }
    }
    if (!http_method) {
        setErrorState(404);
    } else if (http_version != "HTTP/1.1") {
        setErrorState(4); // arbitrary numbers for now
    } else {
        assert((route = findRouteInfo()));
        // Directory where file should be searched from
        // PATH_INFO could be in uri, eg /infile in .../script.cgi/infile
        request_uri.replace(0, request_uri.find(route->prefix_str), route->root);
        recvbuf.erase(0, pos + 2);
        setState(HEADERS);
    }
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
        msg_body = recvbuf.erase(0, pos + delim.length());
        setState(MSG_BODY);
    } else {
        setState(FINISHED);
    }
    return true;
}

bool Client::unchunkRequest() { // only unchunking of request will require a intermediate temp buffer to selectively read bytes into msg_body
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
                return setErrorState(8), true;
            }
            iss.clear();
            if (!bytes) {
                return setPState(FINISHED), true;
            }
            std::advance(it1, delim_length);
            if (std::distance(it1, ite) < bytes) {
                break ; // need more bytes, wait for next recv
            }
            msg_body.insert(msg_body.end(), it1, it1 + bytes);
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
    msg_body.append(buf, bytes);
    bytes_left -= bytes;
    if (!bytes_left) {
        return setPState(FINISHED), true;
    } else {
        return false;
    }
}

void Client::configureIOMethod() {
    static const std::string content_length = "Content-Length", transfer_encoding = "Transfer-Encoding";

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
    // only insert the needed info into msg_body string
    if (p_state == ERROR) {
        return true;
    } else if (track_length) {
        return trackRecvBytes(buf, bytes);
    } else if (unchunk_flag) {
        return unchunkRequest(buf, bytes);
    } else {
        msg_body.append(buf, bytes);
        return false; // only stops when eof is detected during recv call
    }
}
