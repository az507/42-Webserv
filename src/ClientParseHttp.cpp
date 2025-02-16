#include "Client.hpp"

void Client::parseHttpRequest(const char *buf, size_t bytes) {
    int res = 0;

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
            case ERROR:             /*setPState(START_LINE);*/ return ;
            default:                assert(0); // should not reach here
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
    return it == ite ? NULL : &*it;
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
        // consistent error here if cgi has content length set
        std::cerr << "\t_recvbuf: " << _recvbuf << '\n';
        std::cerr << "OPTION A, http_str: " << http_str << ", _requesturi: " << _requesturi<< ", http_version: " << http_version << "\n";
        //assert(0);
        return setErrorState(400), 1;
    }
    for (int i = 0; i < 3; ++i) {
        if (http_str == methods[i]) {
            _httpmethod = 1 << i;  // 1, 2, 4 for GET, POST, DELETE
            break ;
        }
    }
    assert(_recvbuf.length() >= pos + newline.length());
    _recvbuf.erase(0, pos + newline.length());
    if (!_httpmethod) {
        return setErrorState(405), -1; //Method Not Allowed
    } else if (http_version != "HTTP/1.1") {
        return setErrorState(505), -1; //HTTP Version Not Supported
    } else {
        _route = findRouteInfo();
        if (_route && _route->root.empty() && !_route->redirect.empty()) {
            _requesturi = _route->redirect;
            _route = findRouteInfo();
        }
        if (!_route) {
            return setErrorState(404), -1;
        }
        if (!(_httpmethod & _route->http_methods)) {
            return setErrorState(405), -1; // Method not allowed
        }
        std::cout << "Before, _requesturi: " << _requesturi << std::endl;
        char resolvedpath[PATH_MAX];
        if (_route->prefix_str.length() > 1 && realpath(_requesturi.c_str() + 1, resolvedpath)) {
            _requesturi = resolvedpath;
        } else {
            _requesturi.replace(0, _route->prefix_str.length(), _route->root);
        }
        std::cout << "\t_requesturi: " << _requesturi << std::endl;
        std::vector<std::string>::const_iterator it;
        if (!Server::isDirectory(_requesturi)) {
            for (it = _route->cgi_extensions.begin(); it != _route->cgi_extensions.end(); ++it)
            {
                if (_requesturi.find(*it) != std::string::npos)
                {
                    if (access(_requesturi.c_str(), X_OK | F_OK) == -1)
                        return setErrorState(404), -1;
                    break ;
                }
            }
            if (it == _route->cgi_extensions.end() && access(_requesturi.c_str(), F_OK | R_OK) == -1 && _httpmethod != POST_METHOD) {
                return setErrorState(404), -1;
            }
        }
        // Directory where file should be searched from
        // PATH_INFO could be in uri, eg /infile in .../script.cgi/infile
        this->_httpmethod = _httpmethod;
        std::cout << "After, _requesturi: " << _requesturi << '\n' << std::endl;
        setPState(HEADERS);
    }
    return 1;
}

int Client::checkServerName() {
    std::string temp;
    std::map<std::string, std::string>::const_iterator it;

    it = _headers.find("Host");
    if (it != _headers.end() && !_server.names.empty()) {
        for (std::vector<std::string>::const_iterator n_it = _server.names.begin(); n_it != _server.names.end(); ++n_it) {
            if (std::search(it->second.begin(), it->second.end(), n_it->begin(), n_it->end()) != it->second.end()) {
                return 0;
            }
        }
        for (std::vector<std::pair<std::string, std::string> >::const_iterator p_it = _server.ip_addrs.begin(); p_it != _server.ip_addrs.end(); ++p_it) {
            temp = p_it->first + ":" + p_it->second;
            if (temp == it->second) {
                return 0;
            }
        }
        return setErrorState(404), -1;
    }
    return 0;
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
    while (std::getline(iss, buf)) {
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
    //assert(0);
    if (checkServerName() == -1 || !configureIOMethod(_headers)) {
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

bool Client::configureIOMethod(const std::map<std::string, std::string>& headers) {
    std::map<std::string, std::string>::const_iterator it1, it2, ite;

    it1 = headers.find("Content-Length");
    it2 = headers.find("Transfer-Encoding");
    ite = headers.end();

    if (it1 != ite && it2 != ite) {
        return setErrorState(400), false; // bad request, both headers should never exist together
    } else if (it2 != ite && it2->second.find("chunked") != std::string::npos) {
        _unchunkflag = true;
    } else if (it1 != ite) {
        _tracklength = true;
        if (!(std::istringstream(it1->second) >> _bytesleft)) {
            return setErrorState(400), false;
        } else if (ServerInfo::max_bodysize && _bytesleft > ServerInfo::max_bodysize) {
            return setErrorState(413), false;
        }
    }
    return true;
}

/*
bool Client::configureIOMethod(std::map<std::string, std::string> const& _headers) {
    static const std::string content_length = "Content-Length", transfer_encoding = "Transfer-Encoding";

    if (_headers.count(content_length) && _headers.count(transfer_encoding)) {
        std::cerr << "OPTION B\n";
        setErrorState(400);//bad request // both headers should never exist together
        return false;
    }
    if (_headers.count(content_length)) {
        _tracklength = true;
        if (!(std::istringstream(_headers.find(content_length)->second) >> _bytesleft)) { // num too large
            std::cerr << "OPTION C\n";
            setErrorState(400);//bad request
            return false;
        }
        if (_bytesleft > ServerInfo::max_bodysize) {
            return setErrorState(413), false;
        }
    }
    if (_headers.count(transfer_encoding) && _headers.find(transfer_encoding)->second == "chunked") {
        _unchunkflag = true;
    }
    return true;
}
*/

int Client::trackRecvBytes(size_t bytes) {

    assert(bytes <= _bytesleft);
    _bytesleft -= bytes;
    if (!_bytesleft) {
        return setPState(FINISHED), 1;
    } else {
        return 0;
    }
}

// int Client::unchunkRequest() { // only unchunking of request will require a intermediate temp buffer to selectively read bytes into msg_body
//     uint64_t bytes;
//     std::istringstream iss;
//     std::string::const_iterator it1, it2, ite;
//     std::string::const_reverse_iterator r_it1, r_it2, r_ite;
//     static const std::string newline = "\r\n";

//     it1 = it2 = recvbuf.begin();
//     ite = recvbuf.end();
//     r_ite = recvbuf.rend();
//     while (1) {
//         it1 = std::find(it1, ite, '\r');
//         if (it1 == ite || std::distance(it1, ite) < static_cast<std::ptrdiff_t>(newline.length())) {
//             break ;
//         }
//         if (!strncmp(&*it1, newline.c_str(), newline.length())) { // check if its "\r\n"
//             r_it1 = recvbuf.rend() - std::distance(static_cast<std::string::const_iterator>(recvbuf.begin()), it1) - 1;
//             //r_it2 = std::find_if(++r_it1, r_ite, std::unary_negate<std::pointer_to_unary_function<int, int> >(std::ptr_fun(&isdigit)));
//             r_it2 = std::find_if(++r_it1, r_ite, std::not1(std::ptr_fun(&isdigit)));
//             iss.str(std::string(r_it1, r_it2));
//             if (!(iss >> bytes)) { // integer overflow
//                 return setErrorState(8), -1;
//             }
//             iss.clear();
//             if (!bytes) {
//                 return setPState(FINISHED), 1;
//             }
//             std::advance(it1, newline.length());
//             if (std::distance(it1, ite) < static_cast<std::ptrdiff_t>(bytes)) {
//                 break ; // need more bytes, wait for next recv
//             }
//             msg_body.insert(msg_body.end(), it1, it1 + bytes);
//             it1 = std::find(it1, ite, '\n');
//             it2 = it1 + 1; // only after a successful extraction, then we move the it2(old) iterator forward
//         } else {
//             ++it1;
//         }
//     }
//     recvbuf.erase(recvbuf.begin(), recvbuf.end() - std::distance(static_cast<std::string::const_iterator>(recvbuf.begin()), it2));
//     return 0;
// }

int Client::unchunkRequest() 
{
    static const std::string newline = "\r\n";
    size_t pos;

    while (true) 
    {
        // Find the position of the first newline (end of chunk size line)
        pos = _recvbuf.find(newline);
        if (pos == std::string::npos) 
        {
            break; // Wait for more data
        }

        // Parse the chunk size in hexadecimal
        std::istringstream iss(_recvbuf.substr(0, pos));
        size_t chunk_size;
        if (!(iss >> std::hex >> chunk_size)) 
        {
            std::cerr << "OPTION D, chunk_size: " << chunk_size << "\n";
            return setErrorState(400), -1; // Invalid chunk size
        }

        // Remove the chunk size line from the buffer
        _recvbuf.erase(0, pos + newline.length());

        // Check if it's the last chunk
        if (chunk_size == 0) 
        {
            // Look for the final newline (after trailing headers)
            pos = _recvbuf.find(newline);
            if (pos != 0) {
                // Process trailing headers if necessary
                // (Optional: implement trailing headers handling)
            }
            _recvbuf.clear(); // Clear the buffer
            return setPState(FINISHED), 1; // Parsing finished
        }

        // Ensure the buffer contains the full chunk data and trailing \r\n
        if (_recvbuf.size() < chunk_size + newline.length()) 
        {
            break; // Wait for more data
        }

        // Append the chunk data to msg_body
        _msgbody.append(_recvbuf, 0, chunk_size);

        // Remove the chunk data and trailing \r\n from the buffer
        _recvbuf.erase(0, chunk_size + newline.length());
        std::cerr << "Chunk size: " << chunk_size << ", Buffer size: " << _recvbuf.size() << "\n";
        std::cerr << "Chunk data: " << _recvbuf.substr(0, chunk_size) << "\n";
    }
    return 0; // Wait for more data
}
