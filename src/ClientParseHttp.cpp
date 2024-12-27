#include "Client.hpp"

void Client::parseHttpRequest(const char *buf, size_t bytes) {
    int res;

    assert(_pstate != ERROR);
    if (_pstate == MSG_BODY && !_unchunkflag) {
        if (_tracklength) {
            bytes = std::min(_bytesleft, bytes);
        }
        _msgbody.append(buf, bytes);
    } else {
        _recvbuf.append(buf, bytes);
    }
    do {
        switch (_pstate) {
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

    if (_server.routes.empty()) {
        return NULL;
    }
    max_len = 0;
    it = ite = _server.routes.end();
    for (std::vector<RouteInfo>::const_iterator p = _server.routes.begin(); p != ite; ++p) {
        len = p->prefix_str.length();
        findpos = _requesturi.find(p->prefix_str);
        if (findpos == 0 && /*findpos != std::string::npos &&*/ len > max_len) {
            it = p;
            max_len = len;
            std::cout << "\tMAX_LEN = " << max_len << std::endl;
        }
    }
    assert(it != ite);
    return &*it;
}

int Client::parseStartLine() {
    size_t pos;
    int _httpmethod = 0;
    std::istringstream iss;
    std::string http_str, http_version;
    static const std::string newline = "\r\n";
    static const std::string methods[] = { "GET", "POST", "DELETE" };

    pos = _recvbuf.find(newline);
    if (pos == std::string::npos) {
        return 0;
    }
    iss.str(_recvbuf.substr(0, pos));
    if (!(iss >> http_str) || !(iss >> _requesturi) || !(iss >> http_version)) {
        return setErrorState(1), 1;
    }
    for (int i = 0; i < 3; ++i) {
        if (http_str == methods[i]) {
            _httpmethod = 1 << i;  // 1, 2, 4 for GET, POST, DELETE
            break ;
        }
    }
    if (_httpmethod == DELETE_METHOD) {
        std::cerr << "DELETE METHOD ENCOUNTERED, EXITING...\n";
        exit(1);
    }
    assert(_recvbuf.length() >= pos + newline.length());
    _recvbuf.erase(0, pos + newline.length());
    if (!_httpmethod) {
        setErrorState(405);
    } else if (http_version != "HTTP/1.1") {
        setErrorState(4); // arbitrary numbers for now
    } else {
        _route = findRouteInfo();
//        std::cout << "\tPRINTING FOUND ROUTE CONTENTS\n" << std::endl;
//        std::cout << *_route << std::endl;
        assert(_route);
        if (!(_httpmethod & _route->http_methods)) {
            setErrorState(405); // Method not allowed
        }
        // Directory where file should be searched from
        // PATH_INFO could be in uri, eg /infile in .../script.cgi/infile
        this->_httpmethod = _httpmethod;
        std::cout << "Before, _requesturi: " << _requesturi << std::endl;
        //_requesturi.replace(0, _requesturi.find(_route->prefix_str), _route->root);
        //if (_route && _requesturi.find(_route->root) != std::string::npos) {
        char resolvedpath[PATH_MAX];
        if (_route->prefix_str.length() > 1 && realpath(_requesturi.c_str() + 1, resolvedpath)) {
            _requesturi = resolvedpath;
        } else {
            _requesturi.replace(0, _route->prefix_str.length(), _route->root);
        }
//        } else {
//
//        }
        std::cout << "After, _requesturi: " << _requesturi << '\n' << std::endl;
        setPState(HEADERS);
    }
    return 1;
}

int Client::parseHeaders(size_t& bytes) {
    size_t pos, pos1;
    std::string buf;
    std::istringstream iss;
    static const std::string delim = "\r\n\r\n";

    pos = _recvbuf.find(delim);
    if (pos == std::string::npos) {
        return 0;
    }
    iss.str(_recvbuf.substr(0, pos));
    while (!getline(iss, buf).eof()) {
        if (!iss) {
            iss.exceptions(iss.rdstate());
        } else {
            pos1 = buf.find(':');
            if (pos1 != std::string::npos) {
                _headers[buf.substr(0, pos1)] = buf.substr(pos1 + 2, buf.length() - pos1 - 3); // + 2 for "\r\n"
            }
        }
    }
    //std::copy(_headers.begin(), _headers.end(), std::ostream_iterator<std::map<std::string, std::string>::value_type>(std::cout, "\n"));
    if (!configureIOMethod(_headers)) {
        return -1;
    }
    if (_httpmethod == POST_METHOD) {
        if (_unchunkflag) {
            _recvbuf.erase(0, pos + delim.length());
        } else {
            _msgbody.append(_recvbuf.begin() + delim.length() + pos, _recvbuf.end());
            _recvbuf.clear();
            if (_tracklength) {
                assert(_bytesleft >= _msgbody.length());
                bytes = _msgbody.length();
            }
        }
        setPState(MSG_BODY);
    } else {
        setPState(FINISHED);
    }
    return 1;
}

int Client::parseMsgBody(size_t bytes) { // need to handle chunked encoding

    if (_tracklength) {
        return trackRecvBytes(bytes);
    } else if (_unchunkflag) {
        return unchunkRequest();
    } else {
        return 0; // only stops when eof is detected during recv call
    }
}

bool Client::configureIOMethod(std::map<std::string, std::string> const& _headers) {
    static const std::string content_length = "Content-Length", transfer_encoding = "Transfer-Encoding";

    if (_headers.count(content_length) && _headers.count(transfer_encoding)) {
        setErrorState(1); // both _headers should never exist together
        return false;
    }
    if (_headers.count(content_length)) {
        _tracklength = true;
        if (!(std::istringstream(_headers.find(content_length)->second) >> _bytesleft)) { // num too large
            setErrorState(1);
            return false;
        }
    }
    if (_headers.count(transfer_encoding) && _headers.find(transfer_encoding)->second == "chunked") {
        _unchunkflag = true;
    }
    return true;
}

int Client::trackRecvBytes(size_t bytes) {

    assert(bytes <= _bytesleft);
    _bytesleft -= bytes;
    if (!_bytesleft) {
        return setPState(FINISHED), 1;
    } else {
        return 0;
    }
}

int Client::unchunkRequest() { // only unchunking of request will require a intermediate temp buffer to selectively read bytes into _msgbody
    uint64_t bytes;
    std::istringstream iss;
    std::string::const_iterator it1, it2, ite;
    std::string::const_reverse_iterator r_it1, r_it2, r_ite;
    static const std::string newline = "\r\n";

    it1 = it2 = _recvbuf.begin();
    ite = _recvbuf.end();
    r_ite = _recvbuf.rend();
    while (1) {
        it1 = std::find(it1, ite, '\r');
        if (it1 == ite || std::distance(it1, ite) < static_cast<std::ptrdiff_t>(newline.length())) {
            break ;
        }
        if (!strncmp(&*it1, newline.c_str(), newline.length())) { // check if its "\r\n"
            r_it1 = _recvbuf.rend() - std::distance(static_cast<std::string::const_iterator>(_recvbuf.begin()), it1) - 1;
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
            _msgbody.insert(_msgbody.end(), it1, it1 + bytes);
            it1 = std::find(it1, ite, '\n');
            it2 = it1 + 1; // only after a successful extraction, then we move the it2(old) iterator forward
        } else {
            ++it1;
        }
    }
    _recvbuf.erase(_recvbuf.begin(), _recvbuf.end() - std::distance(static_cast<std::string::const_iterator>(_recvbuf.begin()), it2));
    return 0;
}
