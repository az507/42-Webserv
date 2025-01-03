#include "Client.hpp"

void Client::parseHttpRequest(const char *buf, size_t bytes) {
    int res;

    assert(p_state != ERROR);
    if (p_state == MSG_BODY && !unchunk_flag) {
        if (track_length) {
            bytes = std::min(bytes_left, bytes);
        }
        msg_body.append(buf, bytes);
    } else {
        recvbuf.append(buf, bytes);
    }
    do {
        switch (p_state) {
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

    if (server.routes.empty()) {
        return NULL;
    }
    max_len = 0;
    it = ite = server.routes.end();
    for (std::vector<RouteInfo>::const_iterator p = server.routes.begin(); p != ite; ++p) {
        len = p->prefix_str.length();
        findpos = request_uri.find(p->prefix_str);
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
    int http_method = 0;
    std::istringstream iss;
    std::string http_str, http_version;
    static const std::string newline = "\r\n";
    static const std::string methods[] = { "GET", "POST", "DELETE" };

    pos = recvbuf.find(newline);
    if (pos == std::string::npos) {
        return 0;
    }
    iss.str(recvbuf.substr(0, pos));
    if (!(iss >> http_str) || !(iss >> request_uri) || !(iss >> http_version)) {
        return setErrorState(400), 1;
    }
    for (int i = 0; i < 3; ++i) {
        if (http_str == methods[i]) {
            http_method = 1 << i;  // 1, 2, 4 for GET, POST, DELETE
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
        setErrorState(405);
    } else if (http_version != "HTTP/1.1") {
        setErrorState(4); // arbitrary numbers for now
    } else {
        route = findRouteInfo();
//        std::cout << "\tPRINTING FOUND ROUTE CONTENTS\n" << std::endl;
//        std::cout << *route << std::endl;
        assert(route);
        if (!(http_method & route->http_methods)) {
            setErrorState(405); // Method not allowed
        }
        // Directory where file should be searched from
        // PATH_INFO could be in uri, eg /infile in .../script.cgi/infile
        this->http_method = http_method;
        std::cout << "Before, request_uri: " << request_uri << std::endl;
        //request_uri.replace(0, request_uri.find(route->prefix_str), route->root);
        //if (route && request_uri.find(route->root) != std::string::npos) {
        char resolvedpath[PATH_MAX];
        if (route->prefix_str.length() > 1 && realpath(request_uri.c_str() + 1, resolvedpath)) {
            request_uri = resolvedpath;
        } else {
            request_uri.replace(0, route->prefix_str.length(), route->root);
        }
//        } else {
//
//        }
        std::cout << "After, request_uri: " << request_uri << '\n' << std::endl;
        setPState(HEADERS);
    }
    return 1;
}

int Client::parseHeaders(size_t& bytes) {
    size_t pos, pos1;
    std::string buf;
    std::istringstream iss;
    static const std::string delim = "\r\n\r\n";

    pos = recvbuf.find(delim);
    if (pos == std::string::npos) {
        return 0;
    }
    iss.str(recvbuf.substr(0, pos));
    while (!getline(iss, buf).eof()) {
        if (!iss) {
            iss.exceptions(iss.rdstate());
        } else {
            pos1 = buf.find(':');
            if (pos1 != std::string::npos) {
                headers[buf.substr(0, pos1)] = buf.substr(pos1 + 2, buf.length() - pos1 - 3); // + 2 for "\r\n"
            }
        }
    }
    //std::copy(headers.begin(), headers.end(), std::ostream_iterator<std::map<std::string, std::string>::value_type>(std::cout, "\n"));
    if (!configureIOMethod(headers)) {
        return -1;
    }
    if (http_method == POST_METHOD) {
        if (unchunk_flag) {
            recvbuf.erase(0, pos + delim.length());
        } else {
            msg_body.append(recvbuf.begin() + delim.length() + pos, recvbuf.end());
            recvbuf.clear();
            if (track_length) {
                assert(bytes_left >= msg_body.length());
                bytes = msg_body.length();
            }
        }
        setPState(MSG_BODY);
    } else {
        setPState(FINISHED);
    }
    return 1;
}

int Client::parseMsgBody(size_t bytes) { // need to handle chunked encoding

    if (track_length) {
        return trackRecvBytes(bytes);
    } else if (unchunk_flag) {
        return unchunkRequest();
    } else {
        return 0; // only stops when eof is detected during recv call
    }
}

bool Client::configureIOMethod(std::map<std::string, std::string> const& headers) {
    static const std::string content_length = "Content-Length", transfer_encoding = "Transfer-Encoding";

    if (headers.count(content_length) && headers.count(transfer_encoding)) {
        setErrorState(1); // both headers should never exist together
        return false;
    }
    if (headers.count(content_length)) {
        track_length = true;
        if (!(std::istringstream(headers.find(content_length)->second) >> bytes_left)) { // num too large
            setErrorState(1);
            return false;
        }
    }
    if (headers.count(transfer_encoding) && headers.find(transfer_encoding)->second == "chunked") {
        unchunk_flag = true;
    }
    return true;
}

int Client::trackRecvBytes(size_t bytes) {

    assert(bytes <= bytes_left);
    bytes_left -= bytes;
    if (!bytes_left) {
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

    while (true) {
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
    }

    return 0; // Wait for more data
}
