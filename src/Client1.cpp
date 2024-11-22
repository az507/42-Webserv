#include "../include/Client1.hpp"

Client::Client(int fd, std::vector<ServerInfo> const& servers)
    : state(START_LINE), sockfd(fd), http_status(200), server(initServer(servers)), recv_offset(0), send_offset(0) {

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
    parseHttpRequest(std::string(buf, bytes));
}

void Client::socketSend() {
    int bytes;

    assert(state == ERROR || state == SEND_READY);
    if (state != SEND_READY) {
        return ;
    }
    bytes = send(sockfd, sendbuf.c_str() + send_offset, sendbuf.length() - send_offset, MSG_DONTWAIT);
    if (bytes <= 0) {
        return setInvalidState();
    }
}

int Client::getState() const {

    return state;
}

bool Client::parseStartLine() {
    size_t pos;
    std::istringstream iss;
    std::string http_method, http_version;
    std::vector<RouteInfo>::const_iterator ite;

    pos = recvbuf.find("\r\n");
    if (pos == std::string::npos) {
        return true;
    }
    iss.str(recvbuf.substr(0, pos));
    if (!(iss >> http_method) || !(iss >> request_uri) || !(iss >> http_version)) {
        setErrorState(1);
    }
    if (http_method != "GET" && http_method != "POST" && http_method != "DELETE") {
        setErrorState(2);
    }
    ite = server.routes.end();
    for (std::vector<RouteInfo>::const_iterator it = server.routes.begin(); it != ite; ++it) {
        pos = request_uri.find(it->prefix_str, 0);
        if (pos != std::string::npos) {
            request_uri.replace(0, pos, it->root);
            break ;
        }
    }
    if (http_version != "HTTP/1.1") {
        setErrorState(4); // arbitrary numbers for now
    }
    recvbuf.erase(0, pos + 2);
    return false;
}

bool ClientInfo::parseHeaders() {
    size_t pos;
    std::string buf;
    std::stringstream oss;

    pos = recvbuf.find("\r\n\r\n");
    if (pos == std::string::npos) {
        return true;
    }
    oss.str(recvbuf.substr(0, pos));
    while (!getline(oss, buf).eof()) {
        if (!oss) {
            oss.exceptions(oss.rdstate());
        } else {
            pos = str.find(':');
            assert(pos != std::string::npos);
            headers[buf.substr(0, pos)] = buf.substr(pos + 1);
        }
    }
    return false;
}

void Client::parseHttpRequest(std::string const& request) {
    bool is_done;

    recvbuf += request;
    do {
        switch (state) {
            case START_LINE:        is_done = parseStartLine(), ++state, continue ;
            case HEADERS:           is_done = parseHeaders(), ++state, continue ;
            case MSG_BODY:          is_done = parseMsgBody(), ++state, continue ;
            case SEND_READY:
            case ERROR:             break ;
            default:                throw std::exception(); // should not reach here
    } while (!is_done);
}

void Client::setErrorState(int error_nbr) { // prepare error msgs to send back, see if there is file to answer it
    std::string filepath;

    state = ERROR;
    http_code = error_nbr;
    if (ServerInfo::error_pages.count(error_nbr)) {
        filepath = ServerInfo::error_pages[error_nbr];
    } else {

    }
}

void Client::fillSendBuf(int http_code, const char *msg) {
    std::ostringstream oss;

    oss << "HTTP/1.1 " << http_code << ' ' << msg << "\r\n";
    oss << "
}

void Client::setFinishedState() {

    epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, NULL);
    close(sockfd);
    state = FINISHED;
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
