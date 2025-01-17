#include "Client.hpp"

void Client::parseCgiOutput(const char *buf, size_t bytes) {
    int res;

    assert(_pstate != ERROR);
    assert(_iostate == RECV_CGI);
    if (_pstate == MSG_BODY && _unchunkflag) {
        _recvbuf.append(buf, bytes);
    } else {
        if (_pstate == MSG_BODY && _tracklength) {
            bytes = std::min(_bytesleft, bytes);
        }
        _msgbody.append(buf, bytes);
    }
    //std::cout << buf << '\n';
    do {
        switch (_pstate) {
            case START_LINE:    res = ignoreStartLine(); continue ;
            case HEADERS:       res = parseCgiHeaders(bytes); continue ;
            case MSG_BODY:      res = parseMsgBody(bytes); continue ; // assumes CGI output always have msg body
            case FINISHED:      setIOState(SEND_HTTP);
                                close(_clientfd);
                                std::swap(_clientfd = -1, passive_fd);
                                _send_it = _msgbody.begin();
                                _send_ite = _msgbody.end();
                                std::cout << "_msgbody.length(): " << _msgbody.length() << std::endl;
                                return ;
                                //throw "abc123";
            case ERROR:         break ;
            default:            assert(0);
        }
    }
    while (res);
}

int Client::ignoreStartLine() {
    size_t pos;

    pos = _msgbody.find("\r\n");
    if (pos == std::string::npos) {
        return 0;
    } else {
        setPState(HEADERS);
        return 1;
    }
}

int Client::parseCgiHeaders(size_t& bytes) {
    std::string buf;
    size_t pos, pos1;
    std::istringstream iss;
    std::map<std::string, std::string> _headers;
    static const std::string delim = "\r\n\r\n";

    pos = _msgbody.find("\r\n\r\n");
    if (pos == std::string::npos) {
        return 0;
    }
    iss.str(_msgbody.substr(0, pos));
    while (!getline(iss, buf).eof()) {
        if (!iss) {
            iss.exceptions(iss.rdstate());
        } else {
            pos1 = buf.find(':');
            if (pos1 != std::string::npos) {
                //_headers[_recvbuf.substr(0, pos1)] = _recvbuf.substr(pos1 + 1);
                _headers[_msgbody.substr(0, pos1)] = _msgbody.substr(pos1 + 1);
            }
        }
    }
//    setPState(FINISHED);
//    return 1;
    //assert(_recvbuf.empty());
    _tracklength = _unchunkflag = false;
    //assert(!_tracklength && !_unchunkflag);
    if (!configureIOMethod(_headers)) {
        return -1;
    }
    assert(_msgbody.length() >= delim.length() + pos);
    if (_tracklength) {
        bytes = _msgbody.length() - delim.length() - pos;
    } else if (_unchunkflag) {
        _recvbuf.append(_msgbody.begin() + pos + delim.length(), _msgbody.end());
        _msgbody.erase(pos + delim.length());
    }
    setPState(MSG_BODY);
    return 1;

