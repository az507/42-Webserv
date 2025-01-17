#include "Cgi.hpp"

CGI::CGI(int state, int cgifd, int clientfd) :
    _pstate(Client::START_LINE),_iostate(state), _activefd(cgifd), _passivefd(clientfd),
    _unchunkflag(false), _tracklength(false), _recvbytes(0) {}

int CGI::socketSend() {
    ssize_t bytes;

    std::cout << "cgi in socketSend()" << std::endl;
    assert(_iostate == Client::SEND_CGI || _iostate == Client::SEND_HTTP);
    bytes = send(_activefd, &*_send_it, std::distance(_send_it, _send_ite),
        MSG_DONTWAIT);
    switch (bytes) {
        case -1:        //handle_error("send cgi");
        case 0:         if (_iostate == Client::SEND_CGI) {
                            _iostate = Client::RECV_CGI;
                            break ;
                        } else {
                            return setFinishedState(), 0;
                        }
        default:        std::advance(_send_it, bytes);
                        if (_send_it == _send_ite) {
                            if (_iostate == Client::SEND_CGI) {
                                _iostate = Client::RECV_CGI;
                                break ;
                            } else {
                                std::cout << "_activefd: " << _activefd << ", _passivefd: " << _passivefd << std::endl;
                                //assert(0);
                                return setFinishedState(), 0;
                            }
                        }
    }
    return 1;
}

int CGI::getIOState() const {
    return _iostate;
}

bool CGI::operator==(int fd) const {
    return _activefd == fd;
}

void CGI::cleanup() {
    if (_activefd != -1 && _passivefd != -1) {
        Client::deleteEvent(_activefd);
        close(_activefd);
    }
    _activefd = _passivefd = -1;
}

void CGI::socketRecv() {
    ssize_t bytes;
    char buf[BUFSIZE + 1];

    //std::cout << "cgi in socketRecv()" << std::endl;
    assert(_iostate == Client::RECV_CGI);
    bytes = recv(_activefd, buf, BUFSIZE, MSG_DONTWAIT);
    //std::cout << "bytes recv'd in cgi: " << bytes << std::endl;
    if (bytes > 0) {
        buf[bytes] = '\0';
    }
    //std::cout << buf << std::endl;
    switch (bytes) {
        case -1:        //handle_error("recv cgi");
        case 0:         setClientReady(); break ;
        default:        parseCgiOutput(buf, bytes);
    }
}

std::pair<int, int> CGI::getFds() const {
    return std::make_pair(_activefd, _passivefd);
}

void CGI::setClientReady() {
    _pstate = Client::FINISHED;
    _iostate = Client::SEND_HTTP;
    _send_it = _clientbuf.begin();
    _send_ite = _clientbuf.end();
    assert(_activefd != -1 && _passivefd != -1);
    Client::deleteEvent(_activefd);
    close(_activefd);
    std::swap(_passivefd, _activefd = -1);
}

void CGI::setErrorState(int httpcode) {
    std::ifstream infile;
    std::ostringstream oss;
    std::map<int, std::string>::const_iterator it;

    it = ServerInfo::error_pages.find(httpcode);
    if (it != ServerInfo::error_pages.end() && !access(it->second.c_str(), F_OK | R_OK)) {
        oss << "HTTP/1.1 " << httpcode << Client::getHttpStatus(httpcode) << "\r\n";
        oss << "Content-Length: " << Client::getContentLength(it->second) << "\r\n";
        oss << "Content-Type: text/html\r\n\r\n";
        _clientbuf = oss.str();
        infile.open(it->second.c_str());
        assert(infile.is_open());
        std::copy(std::istreambuf_iterator<char>(infile),
            std::istreambuf_iterator<char>(), std::back_inserter(_clientbuf));
    }
    _send_it = _clientbuf.begin();
    _send_ite = _clientbuf.end();
    _pstate = Client::ERROR;
    _iostate = Client::SEND_HTTP;
    assert(_activefd != -1 && _passivefd != -1);
    Client::deleteEvent(_activefd);
    close(_activefd);
    std::swap(_activefd = -1, _passivefd);
}

void CGI::setFinishedState() {
    _iostate = Client::CONN_CLOSED;
    //close(_activefd);
    _activefd = -1;
}

void CGI::parseCgiOutput(const char *buf, ssize_t bytes) {
    int res;

    assert(bytes > 0);
    if (_pstate == Client::MSG_BODY && _tracklength) {
        bytes = std::min(_recvbytes, (size_t)bytes);
        _clientbuf.append(buf, bytes);
        //std::cout << "parseCgiOutput with _tracklength, bytes: " << bytes << std::endl;
        if (!_recvbytes) {
            setClientReady();
        }
        return ;
    } else {
        _clientbuf.append(buf, bytes);
    }
    do {
        switch (_pstate) {
            case Client::START_LINE:    res = ignoreStartLine(); break ;
            case Client::HEADERS:       res = parseCgiHeaders(); break ;
            case Client::MSG_BODY:      res = parseMsgBody(); break ;
            case Client::FINISHED:      return setClientReady();
            default:                    assert(0);
        }
    }
    while (res);
}

int CGI::ignoreStartLine() {
    return _clientbuf.find("\r\n") == std::string::npos ? 0 : ++_pstate;
}

int CGI::parseCgiHeaders() {
    std::string line;
    size_t pos1, pos2;
    std::istringstream iss;

    pos1 = _clientbuf.find("\r\n\r\n");
    if (pos1 == std::string::npos) {
        return 0;
    }
    iss.str(_clientbuf.substr(0, pos1 + 2));
    while (std::getline(iss, line)) {
        if (!iss) {
            iss.exceptions(iss.rdstate());
        } else {
            pos2 = line.find(':');
            if (pos2 != std::string::npos) {
                _headers[line.substr(0, pos2)] = line.substr(pos2 + 2, line.length() - pos2 - 3);
            }
        }
    }
    configureIOHandling();
    return _tracklength == true ? 0 : 1;
}

void CGI::configureIOHandling() {
    std::map<std::string, std::string>::const_iterator it1, it2, ite;

    it1 = _headers.find("Content-Length");
    it2 = _headers.find("Transfer-Encoding");
    ite = _headers.end();

    if (it1 != ite && it2 != ite) {
        return setErrorState(400);
    } else if (it1 != ite) {
        _tracklength = true;
        if (!(std::istringstream(it1->second) >> _recvbytes)) {
            return setErrorState(400);
        }
        _recvbytes -= std::min(_recvbytes, (_clientbuf.length() - (_clientbuf.find("\r\n\r\n") + 4)));
        if (!_recvbytes) {
            return setClientReady();
        }
    } else if (it2 != ite && it2->second == "chunked") {
        _unchunkflag = true;
    }
    _pstate = Client::MSG_BODY;
}

int CGI::parseMsgBody() {
    assert(!_tracklength);
    return _unchunkflag ? unchunkMsgBody() : 0;
}

// unfinished for now
int CGI::unchunkMsgBody() {
//    uint64_t bytes;
//    std::string temp;
//    std::string::const_iterator it, ite;
    static const std::string newline = "\r\n\r\n";

    return 1;
//    ite = _clientbuf.end();
//    it = std::find_end(_clientbuf.begin(), ite, newline.begin(), newline.end());
//    assert(it != ite);
//    std::advance(it, newline.length());
//    while (it != ite) {
//        it2 = std::find_end(it, ite, newline.begin(), newline.begin() + 2);
//        if (it2 == ite) {
//            break ;
//        }
//        it1 = std::find_if(it, it2, std::ptr_fun(&isdigit));
//        if (it1 == it2) {
//            break ;
//        }
//        if (!std::ostringstream(std::string(it1, it2)) >> bytes) {
//            return setErrorState(2), -1;
//        }
//        if (std::distance(it2 + 2, ite) < bytes) {
//            break ;
//        }
//        std::advance(it2, 2);
//        std::copy(it2, it2 + bytes, std::back_inserter(temp));
//    }
//    return 1;
}

void CGI::setCgiInput(std::string const& buf) {
    _cgistdin = buf;
    _send_it = _cgistdin.begin();
    _send_ite = _cgistdin.end();
}
