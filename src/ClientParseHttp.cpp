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
    iss.str(recvbuf.substr(0, pos));
    if (!(iss >> http_str) || !(iss >> request_uri) || !(iss >> http_version)) {
        return setErrorState(400), 1;
    }
    for (int i = 0; i < 3; ++i) {
        if (http_str == methods[i]) {
            _httpmethod = 1 << i;  // 1, 2, 4 for GET, POST, DELETE
            break ;
        }
    }
//    if (http_method == DELETE_METHOD) {
//        std::cerr << "DELETE METHOD ENCOUNTERED, EXITING...\n";
//        exit(1);
//    }
    assert(recvbuf.length() >= pos + newline.length());
    recvbuf.erase(0, pos + newline.length());
    if (!http_method) {
        setErrorState(405); //Method Not Allowed
    } else if (http_version != "HTTP/1.1") {
        setErrorState(505); //HTTP Version Not Supporte
    } else {
        _route = findRouteInfo();
//        std::cout << "\tPRINTING FOUND ROUTE CONTENTS\n" << std::endl;
//        std::cout << *_route << std::endl;
        assert(_route);
        if (!(_httpmethod & _route->http_methods)) {
            setErrorState(405); // Method not allowed
        }
        std::vector<std::string>::const_iterator it;
        for (it = route->cgi_extensions.begin(); it != route->cgi_extensions.end(); ++it)
        {
            if (request_uri.find(*it) != std::string::npos)
            {
                if (access(request_uri.c_str(), X_OK | F_OK) == -1)
                    return setErrorState(404), -1;
                break ;
            }
        }
        if (it == route->cgi_extensions.end() && access(request_uri.c_str(), F_OK | R_OK) == -1) {
            return setErrorState(404), -1;
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
    while (!std::getline(iss, buf).eof()) {
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

    if (headers.count(content_length) && headers.count(transfer_encoding)) {
        setErrorState(400);//bad request // both headers should never exist together
        return false;
    }
    if (headers.count(content_length)) {
        track_length = true;
        if (!(std::istringstream(headers.find(content_length)->second) >> bytes_left)) { // num too large
            setErrorState(400);//bad request
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
        pos = recvbuf.find(newline);
        if (pos == std::string::npos) 
        {
            break; // Wait for more data
        }

        // Parse the chunk size in hexadecimal
        std::istringstream iss(recvbuf.substr(0, pos));
        size_t chunk_size;
        if (!(iss >> std::hex >> chunk_size)) 
        {
            return setErrorState(400), -1; // Invalid chunk size
        }

        // Remove the chunk size line from the buffer
        recvbuf.erase(0, pos + newline.length());

        // Check if it's the last chunk
        if (chunk_size == 0) 
        {
            // Look for the final newline (after trailing headers)
            pos = recvbuf.find(newline);
            if (pos != 0) {
                // Process trailing headers if necessary
                // (Optional: implement trailing headers handling)
            }
            recvbuf.clear(); // Clear the buffer
            return setPState(FINISHED), 1; // Parsing finished
        }

        // Ensure the buffer contains the full chunk data and trailing \r\n
        if (recvbuf.size() < chunk_size + newline.length()) 
        {
            break; // Wait for more data
        }

        // Append the chunk data to msg_body
        msg_body.append(recvbuf, 0, chunk_size);

        // Remove the chunk data and trailing \r\n from the buffer
        recvbuf.erase(0, chunk_size + newline.length());
        std::cerr << "Chunk size: " << chunk_size << ", Buffer size: " << recvbuf.size() << "\n";
        std::cerr << "Chunk data: " << recvbuf.substr(0, chunk_size) << "\n";
    }
    return 0; // Wait for more data
}
