#include "Client.hpp"

void Client::parseHttpRequest(const char *buf, size_t bytes) {
    bool res;

    do {
        switch (p_state) {
            case START_LINE:        if (res = parseStartLine(buf, bytes)) locateRequestUri(); continue ;
            case HEADERS:           res = parseHeaders(buf, bytes), continue ;
            case MSG_BODY:          res = parseMsgBody(buf, bytes), continue ; //GET REQUEST==DONT ENTER HERE
            case ERROR:
            case FINISHED:          res = prepareResource(), continue ;
            default:                throw std::exception(); // should not reach here
        }
    }
    while (res);
}

void Client::locateRequestUri() {


}

bool Client::parseStartLine(const char *buf, size_t bytes) {
    std::istringstream iss;
    std::string http_str, http_version;
    static const std::string methods[] = { "GET", "POST", "DELETE" };

    pos = recvbuf.append(buf, bytes).find("\r\n");
    if (pos == std::string::npos) {
        return false;
    }
    iss.str(recvbuf.substr(0, pos));
    if (!(iss >> http_str) || !(iss >> request_uri) || !(iss >> http_version)) {
        setErrorState(1);
    }
    for (int i = 0; i < 3; ++i) {
        if (http_str == methods[i]) {
            http_method = i + 1;
            break ;
        }
    }
    if (!http_method) {
        setErrorState(404);
    } else if (http_version != "HTTP/1.1") {
        setErrorState(4); // arbitrary numbers for now
    }
    return true;
}


    locateRequestUri();

    route = findRoute();
    if (route) {
        // Directory where file should be searched from
        // PATH_INFO could be in uri, eg /infile in .../script.cgi/infile
        request_uri.replace(0, request_uri.find(route->prefix_str), route->root);
    }
                                           //
    recvbuf.erase(0, pos + 2);
    setState(HEADERS);
    return true;
