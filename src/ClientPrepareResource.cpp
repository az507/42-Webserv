#include "Client.hpp"

bool Client::performRequest() {
    std::pair<std::string, std::string> reqInfo;

    writeInitialPortion();
    reqInfo = getReqInfo();
    switch (http_method) {
        case GET_METHOD:
        case POST_METHOD:
        case DELETE_METHOD:
        default:                    std::terminate();
    }
}

std::pair<std::string, std::string> Client::filterRequestUri() {
    size_t pos1, pos2;
    std::string::iterator it1, it2, ite;
    std::pair<std::string, std::string> reqInfo;

    it1 = request_uri.begin();
    ite = request_uri.end();
    it2 = std::find(request_uri.begin(), ite, '?');
    if (it2 != ite) {
        reqInfo.first = std::string(it2, ite);
        *it2 = '\0';
    }
    while (it2 != it1) {
        it2 = std::find(
        if (access(request_uri.c_str(), F_OK) == 0) {
            break ;
        }
    }
}

bool Client::prepareResource() {
    int pipefds[2];
    size_t pos1, pos2;
    std::string query_str, pathinfo;

    writeInitialPortion();
    pos1 = request_uri.find('?');
    if (pos1 != std::string::npos) {
        query_str = request_uri.substr(pos + 1);
        request_uri[pos] = '\0';
    }
    // Executing CGI script below
    if (route && !route.cgi_extension.empty()
        && (pos2 = request_uri.find(route.cgi_extension)) != std::string::npos) {

        pathinfo = request_uri.substr(pos2 + route.cgi_extension.length() + 1);
        request_uri[pos2] = '\0';
        if (socketpair(AF_LOCAL, SOCK_STREAM, 0, pipefds) == -1) {
            handle_error("socketpair");
        }
        switch (fork()) {
            case -1:        handle_error("fork");

            case 0:         close(pipefds[1]);
                            executeCgi(pipefds[0], query_str, pathinfo);
                            throw std::exception(); // force stack unwinding of child process

            default:        close(pipefds[0]);
                            registerEvent(pipefds[1], EPOLLIN | EPOLLOUT);
        }
        std::swap(active_fd, passive_fd = pipefds[1]);
        send_it = msg_body.begin();
        send_ite = msg_body.end();
        setIOState(SEND_CGI);

    } else {
        if (!writeToFilebuf(request_uri)) {
            return true;
        }
        send_it = filebuf.begin();
        send_ite = filebuf.end();
        setIOState(SEND_HTTP);
    }
    return false;
}

void Client::writeInitialPortion() {
    std::ostringstream oss;

    assert(Client::http_statuses.count(http_code));
    oss << "HTTP/1.1 " << http_code << ' ' << Client::http_statuses[http_code] << "\r\n";
    oss << "Content-Type: " << getContentType() << "\r\n\r\n";
    filebuf = oss.str();
}

void Client::registerEvent(int fd, uint32_t events) {
    struct epoll_event ev;

    memset(&ev, 0, sizeof(ev));
    ev.data.fd = fd;
    ev.events = events;
    if (epoll_ctl(Client::epollfd, EPOLL_CTL_ADD, fd, ev) == -1) {
        handle_error("RE: epoll_ctl");
    }
}

bool Client::writeToFilebuf(std::string const& filename) {
    const char *filestr;
    std::ifstream infile;
    
    filestr = filename.c_str();
    if (access(filestr, F_OK) == -1) {
        return setErrorState(404), false;
    }
    infile.open(filestr);
    if (!infile.is_open()) {
        return setErrorState(403), false;
    }
    std::copy((std::istreambuf_iterator<char>(infile)),
        std::istreambuf_iterator<char>(), std::back_inserter(filebuf));
    return true;
}
