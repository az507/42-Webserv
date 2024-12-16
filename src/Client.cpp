#include "Client.hpp"

int Client::epollfd = 0;
const char **Client::envp = NULL;

Client::Client(int connfd, std::vector<ServerInfo> const& servers) :
        p_state(START_LINE),
        io_state(RECV_HTTP) ,
        http_code(200),
        http_method(0),
        active_fd(-1),
        passive_fd(-1),
        unchunk_flag(false),
        track_length(false),
        bytes_left(0),
        route(NULL),
        server(initServer(connfd, servers)) {
        
}

bool Client::operator!=(int sockfd) const {

    return !(*this == sockfd);
}

bool Client::operator==(int sockfd) const {

    return sockfd == active_fd;
}

std::ostream& operator<<(std::ostream& os, Client const& obj) {
    static const char *io_states[] = {
        "RECV_HTTP",
        "SEND_HTTP",
        "RECV_CGI",
        "SEND_CGI",
        "CONN_CLOSED",
    };
    static const char *parse_states[] = { // starts from -1
        "ERROR",
        "START_LINE",
        "HEADERS",
        "MSG_BODY",
        "FINISHED",
    };

    //os << "p_state nbr: " << obj.p_state << '\n';
    os << "p_state: " << parse_states[obj.p_state + 1] << '\n';
    //os << "io_state nbr: " << obj.io_state << '\n';
    os << "io_state: " << io_states[obj.io_state] << '\n';
    os << "http_code: " << obj.http_code << '\n';
    os << "http_method: " << obj.http_method << '\n';
    os << "active_fd: " << obj.active_fd << '\n';
    os << "passive_fd: " << obj.passive_fd << '\n';
    os << std::boolalpha;
    os << "unchunk_flag: " << obj.unchunk_flag << '\n';
    os << "track_length: " << obj.track_length << '\n';
    os << "bytes_left: " << obj.bytes_left << '\n';
    os << "recvbuf: " << obj.recvbuf << '\n';
    os << "filebuf: " << obj.filebuf << '\n';
    os << "msg_body: " << obj.msg_body << '\n';
    os << "request_uri: " << obj.request_uri << '\n';
    // os << obj.route << '\n';
    // os << obj.server << '\n';
    os << "send_ite - send_it: " << std::distance(obj.send_it, obj.send_ite) << '\n';
    std::copy(obj.headers.begin(), obj.headers.end(),
        std::ostream_iterator<std::map<std::string, std::string>::value_type>(os, "\n\n"));
    return os;
}

// inside parseHttpRequest(), it can call prepareResource() which would change this->io_state to either
// SEND_RECV or SEND_HTTP, meaning that not all data from socket may be read before starting to send data
// to socket. This could potentially cause problems where socket buffer should be drained first before
// sending data to client socket
void Client::socketRecv() {
    ssize_t bytes;
    char buf[BUFSIZE + 1];

    if (io_state != RECV_HTTP && io_state != RECV_CGI) {
        return ;
    }
    //assert(io_state != RECV_CGI);
    bytes = recv(active_fd, buf, BUFSIZE, MSG_DONTWAIT);
    switch (bytes) {
        case -1:        handle_error("recv");
        case 0:         if (io_state == RECV_HTTP) { // client closed connection ?
                            //std::terminate();
                            closeConnection();
                            //std::cout << "waiting..." << std::endl;
                            /*pid_t pid = *///wait(NULL);
//                            std::cout << "pid = " << pid << ", exiting..." << std::endl;
//                            exit(1);
                        } else {
                            std::cerr << "MSG_BODY LENGTH = " << msg_body.length() << '\n';
                            send_it = msg_body.begin();
                            send_ite = msg_body.end();
                            close(active_fd);
                            std::swap(active_fd = -1, passive_fd);
                            setIOState(SEND_HTTP);      // still unsure about this part
                        }
                        break ;
        default:        buf[bytes] = '\0';
                        std::cout << buf << '\n';
//                        if (io_state == RECV_CGI) {
//                            throw std::exception();
//                        }
                        switch (io_state) {
                            case RECV_HTTP:     parseHttpRequest(buf, bytes); break ;
                            case RECV_CGI:      parseCgiOutput(buf, bytes); break ; // both parse funcs may change IOState
                            default:            std::terminate();
                        }
    }
}

void Client::socketSend() {
    ssize_t bytes;
    std::ptrdiff_t dist;

//    std::cout << "io_state in socketSend(): " << io_state << std::endl;
    if (io_state != SEND_HTTP && io_state != SEND_CGI) {
        return ;
    }
    dist = std::distance(send_it, send_ite);
    //assert(dist > 0);
    bytes = send(active_fd, &*send_it, dist, MSG_DONTWAIT);
    if (io_state == SEND_HTTP) {
        std::cout << "sending to client" << std::endl;
    } else if (io_state == SEND_CGI) {
        std::cout << "sending to CGI" << std::endl;
    }
    std::cout << "bytes sent: " << bytes << ", dist: " << dist << std::endl;
    switch (bytes) {
        // send returns -1 when broken pipe for cgi, SIGPIPE gets sent, for now assume that connection is broken
        case -1:
        case 0:         perror("send");
                        if (io_state == SEND_HTTP) {
                            closeConnection();
                        } else {
                            setIOState(RECV_CGI);
                        }
                        break ;
        default:        advanceSendIterators(bytes);
    }
}

void Client::advanceSendIterators(size_t bytes) {

    std::advance(send_it, bytes);
    if (io_state == SEND_CGI) {
        std::cout << "SENT TO CGI, bytes = " << bytes << std::endl;
        std::cout << "after sending to CGI: active_fd: " << active_fd << ", passive_fd: " << passive_fd << std::endl;
        //throw std::string("abc");
    }
    if (send_it == send_ite) {
        switch (io_state) {
            case SEND_CGI:
                                {
                                    setIOState(RECV_CGI);
                                    break ;
                                }
            case SEND_HTTP:     
                                {
                                    assert(headers.count("Connection"));
                                    std::string const& ref = headers["Connection"];
                                    if (ref == "close") {
                                        closeConnection();
                                    } else if (ref == "keep-alive") {
                                        resetSelf();
                                        setIOState(RECV_HTTP);
                                    }
                                    break ;
                                }
            default:            std::terminate();
        }
    }
}

void Client::closeConnection() {

    deleteEvent(active_fd);
    deleteEvent(passive_fd);
    close(active_fd);
    close(passive_fd);
    p_state = START_LINE;
    io_state = RECV_HTTP;
    http_code = 200;
    http_method = 0;
    active_fd = -1;
    passive_fd = -1;
    unchunk_flag = false;
    track_length = false;
    bytes_left = 0;
    recvbuf.clear();
    filebuf.clear();
    msg_body.clear();
    request_uri.clear();
    route = NULL;
    headers.clear();
    //memset(this, 0, sizeof(*this));
    setIOState(CONN_CLOSED);
}

void Client::resetSelf() {

    if (passive_fd != -1) {
        close(passive_fd);
        passive_fd = -1;
    }
    recvbuf.clear();
    filebuf.clear();
    msg_body.clear();
    request_uri.clear();
    route = NULL;
    send_it = send_ite = filebuf.end();
    headers.clear();
}

void Client::deleteEvent(int fd) {

    if (fd > 0) {
        if (epoll_ctl(Client::epollfd, EPOLL_CTL_DEL, fd, NULL) == -1) {
            handle_error("del: epoll_ctl");
        }
    }
}

void Client::setErrorState(int num) {
    std::map<int, std::string>::const_iterator it;

    http_code = num;
    setPState(ERROR);
    setIOState(SEND_HTTP);
    it = ServerInfo::error_pages.find(http_code);
    if (it != ServerInfo::error_pages.end() && access(it->second.c_str(), F_OK | R_OK) == 0) {
        request_uri = it->second;
        writeInitialPortion();
        writeToFilebuf(request_uri);
    }
    send_it = filebuf.begin();
    send_ite = filebuf.end();
}

ServerInfo const& Client::initServer(int connfd, std::vector<ServerInfo> const& servers) const {
    std::vector<ServerInfo>::const_iterator it, ite;

    for (it = servers.begin(), ite = servers.end(); it != ite; ++it) {
        //std::copy(it->connfds.begin(), it->connfds.end(), std::ostream_iterator<int>(std::cout, "\n"));
        if (std::find_if(it->connfds.begin(), it->connfds.end(), std::bind2nd(std::equal_to<int>(), connfd)) != it->connfds.end()) {
            break ;
        }
    }
    assert(it != ite); // should never reach end
    return *it;
}

std::string Client::getContentType(std::string const& filename) const {
    size_t pos;
    std::map<std::string, std::string>::const_iterator it;
    static const std::map<std::string, std::string> contentTypes( Client::initContentTypes() );

    pos = filename.find_last_of('.');
    if (pos == std::string::npos) {
        return "text/plain";
    } else {
        it = contentTypes.find(filename.substr(pos + 1));
        if (it == contentTypes.end()) {
            return "application/octet-stream"; // treat as binary
        } else {
            return it->second;
        }
    }
}

std::string Client::getHttpStatus(int http_code) const {
    std::map<int, std::string>::const_iterator it;
    static const std::map<int, std::string> httpStatuses( Client::initHttpStatuses() );

    it = httpStatuses.find(http_code);
    if (it == httpStatuses.end()) {
        return "Undefined";
    } else {
        return it->second;
    }
}

//Client::Client() : {}

Client::~Client() {}

//Client::Client(Client const&) {}

Client& Client::operator=(Client const&) { return *this; }
