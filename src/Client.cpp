#include "Client.hpp"

int Client::epollfd = 0;
const char **Client::envp = NULL;
std::vector<int> Client::open_fds;

Client::Client(int connfd, int sockfd, std::vector<ServerInfo> const& _servers) :
        _pstate(START_LINE),
        _iostate(RECV_HTTP) ,
        _httpcode(200),
        _httpmethod(0),
        _clientfd(sockfd),
        _lastresponsetime(time(NULL)),
        _unchunkflag(false),
        _tracklength(false),
        _bytesleft(0),
        _route(NULL),
        _server(initServer(connfd, _servers)) {
        
}

bool Client::operator!=(int sockfd) {
    return !(*this == sockfd);
}

bool Client::operator==(int eventfd) {
    if (!_cgis.empty()) {
        _currptr = std::find_if(_cgis.begin(), _cgis.end(), std::bind2nd(std::mem_fun_ref(&CGI::operator==), eventfd));
    }
    return _currptr != _cgis.end() || eventfd == _clientfd;
}

std::ostream& operator<<(std::ostream& os, Client const& obj) {
    static const char *_iostates[] = {
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

    //os << "_pstate nbr: " << obj._pstate << '\n';
    os << "_pstate: " << parse_states[obj._pstate + 1] << '\n';
    //os << "_iostate nbr: " << obj._iostate << '\n';
    os << "_iostate: " << _iostates[obj._iostate] << '\n';
    os << "_httpcode: " << obj._httpcode << '\n';
    os << "_httpmethod: " << obj._httpmethod << '\n';
    os << "_clientfd: " << obj._clientfd << '\n';
    os << std::boolalpha;
    os << "_unchunkflag: " << obj._unchunkflag << '\n';
    os << "_tracklength: " << obj._tracklength << '\n';
    os << "_bytesleft: " << obj._bytesleft << '\n';
    os << "_recvbuf: " << obj._recvbuf << '\n';
    os << "_filebuf: " << obj._filebuf << '\n';
    os << "_msgbody: " << obj._msgbody << '\n';
    os << "_requesturi: " << obj._requesturi << '\n';
    // os << obj._route << '\n';
    // os << obj._server << '\n';
    os << "_send_ite - _send_it: " << std::distance(obj._send_it, obj._send_ite) << '\n';
    std::copy(obj._headers.begin(), obj._headers.end(),
        std::ostream_iterator<std::map<std::string, std::string>::value_type>(os, "\n\n"));
    return os;
}

// inside parseHttpRequest(), it can call prepareResource() which would change this->_iostate to either
// SEND_RECV or SEND_HTTP, meaning that not all data from socket may be read before starting to send data
// to socket. This could potentially cause problems where socket buffer should be drained first before
// sending data to client socket
void Client::socketRecv() {
    ssize_t bytes;
    char buf[BUFSIZE + 1];

    assert(_iostate != RECV_CGI);
    if (_iostate == RECV_HTTP && _cgis.empty()) {
        bytes = recv(_clientfd, buf, BUFSIZE, MSG_DONTWAIT);
        switch (bytes) {
            case -1:        //handle_error("client recv");
            case 0:         /* closeCgis(), */closeConnection(); break ;
            default:        buf[bytes] = '\0';
                            std::cout << "bytes recv'ed = " << bytes << std::endl;
                            std::cout << buf << std::endl;
                            parseHttpRequest(buf, bytes); _lastresponsetime = time(NULL); break ;
        }
    } else if (!_cgis.empty() && _currptr != _cgis.end() && _currptr->getIOState() == RECV_CGI) {
        _currptr->socketRecv();
    } else {
        //assert(0);
    }
}

void Client::printFds() const {
    std::cout << "_clientfd: " << _clientfd << ", ";
    if (_cgis.empty()) {
        std::cout << "(no cgi fd here)";
    } else {
        int idx = -1;
        for (std::list<CGI>::const_iterator it = _cgis.begin(); it != _cgis.end(); ++it) {
            if (&*it != &*_currptr) {
                std::cout << "_currptr: ";
            }
            std::cout << ++idx << ") " << it->getFds();
        }
    }
    std::cout << std::endl;
}

void Client::socketSend() {
    ssize_t bytes;
    std::map<std::string, std::string>::const_iterator it;

//    std::cout << "parent in socketSend, _iostate = " << _iostate << ", fds: ";
//    printFds();
    assert(_iostate != SEND_CGI);
    if (_iostate == SEND_HTTP && _cgis.empty()) {
        std::ptrdiff_t dist;
        bytes = send(_clientfd, &*_send_it, dist = std::distance(_send_it, _send_ite), MSG_DONTWAIT);
        switch (bytes) {
            case -1:        perror("send client"); closeConnection(); break ;
            case 0:         closeConnection(); break ;
            default:        std::advance(_send_it, bytes);
                            if (_send_it == _send_ite) {
                                if (_pstate == ERROR) {
                                    setPState(START_LINE);
                                }
                                it = _headers.find("Connection");
                                if (it == _headers.end() || it->second == "close") {
                                    closeConnection();
                                } else if (it->second == "keep-alive") {
                                    resetSelf();
                                } else {
                                    assert(0);
                               }
                            }
        }
    } else if (!_cgis.empty() && _currptr != _cgis.end()
        && (_currptr->getIOState() == SEND_HTTP || _currptr->getIOState() == SEND_CGI)) {
        if (!_currptr->socketSend()) {
            _cgis.erase(_currptr);
            _currptr = _cgis.end();
        }
    } else {
        //std::cout << "bypassing Client::socketSend() for now" << std::endl;
        //assert(0);
    }
}

void Client::closeConnection() {
    Client::deleteEvent(_clientfd);
    close(_clientfd);
    _clientfd = -1;
    _pstate = START_LINE;
    _iostate = RECV_HTTP;
    _httpcode = 200;
    _httpmethod = 0;
    _unchunkflag = false;
    _tracklength = false;
    _bytesleft = 0;
    _recvbuf.clear();
    _filebuf.clear();
    _msgbody.clear();
    _requesturi.clear();
    _route = NULL;
    _headers.clear();
    //memset(this, 0, sizeof(*this));
//    std::for_each(_cgis.begin(), _cgis.end(), std::mem_fun_ref(&CGI::cleanup));
//    _cgis.clear();
//    _currptr = _cgis.end();
    setIOState(CONN_CLOSED);
}

void Client::resetSelf() {
    _recvbuf.clear();
    _filebuf.clear();
    _msgbody.clear();
    _requesturi.clear();
    _route = NULL;
    _send_it = _send_ite = _filebuf.end();
    _headers.clear();
    std::for_each(_cgis.begin(), _cgis.end(), std::mem_fun_ref(&CGI::cleanup));
    _cgis.clear();
    _currptr = _cgis.end();
    setPState(START_LINE);
    setIOState(RECV_HTTP);
}

void Client::deleteEvent(int fd) {
    if (fd != -1) {
        if (epoll_ctl(Client::epollfd, EPOLL_CTL_DEL, fd, NULL) == -1) {
            handle_error("del: epoll_ctl");
        }
    }
}

void Client::setErrorState(int num) {
    std::map<int, std::string>::const_iterator it;

    _httpcode = num;
    setPState(ERROR);
    setIOState(SEND_HTTP);
    it = ServerInfo::error_pages.find(_httpcode);
    if (it != ServerInfo::error_pages.end() && access(it->second.c_str(), F_OK | R_OK) == 0) {
        _requesturi = it->second;
        writeInitialPortion();
        writeToFilebuf(_requesturi);
    }
    _send_it = _filebuf.begin();
    _send_ite = _filebuf.end();
}

ServerInfo const& Client::initServer(int connfd, std::vector<ServerInfo> const& servers) const {
    std::vector<ServerInfo>::const_iterator it, ite;

    for (it = servers.begin(), ite = servers.end(); it != ite; ++it) {
        //std::copy(it->connfds.begin(), it->connfds.end(), std::ostream_iterator<int>(std::cout, "\n"));
        //if (std::find_if(it->connfds.begin(), it->connfds.end(), std::bind2nd(std::equal_to<int>(), connfd)) != it->connfds.end()) {
        if (std::find(it->connfds.begin(), it->connfds.end(), connfd) != it->connfds.end()) {
            break ;
        }
    }
    assert(it != ite); // should never reach end
    return *it;
}

std::string Client::getContentType(std::string const& filename) {
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

std::string Client::getHttpStatus(int _httpcode) {
    std::map<int, std::string>::const_iterator it;
    static const std::map<int, std::string> httpStatuses( Client::initHttpStatuses() );

    it = httpStatuses.find(_httpcode);
    if (it == httpStatuses.end()) {
        return "Undefined";
    } else {
        return it->second;
    }
}

void Client::addOpenFd(int fd) {
    open_fds.push_back(fd);
}

void Client::addOpenFds(const std::vector<int>& vec) {
    open_fds.insert(open_fds.end(), vec.begin(), vec.end());
}

void Client::closeOpenFds() {
    std::for_each(open_fds.begin(), open_fds.end(), &close);
    open_fds.clear();
}

//Client::Client() : {}

Client::~Client() {}

//Client::Client(Client const&) {}

Client& Client::operator=(Client const&) { return *this; }
