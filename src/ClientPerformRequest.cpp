#include "Client.hpp"

// return 0 to break out of outer do-while loop (to indicate ready to start sending msgs), non-zero for error
int Client::performRequest() {
    int res = 0;
    std::pair<std::string, std::string> reqInfo;

    reqInfo = filterRequestUri();
    writeInitialPortion();
    if (reqInfo.second.empty()) { // no cgi-extension found
        switch (http_method) {
            case GET_METHOD:            res = performGetMethod(); break ;
            case POST_METHOD:           res = performPostMethod(); break ;
            case DELETE_METHOD:         res = performDeleteMethod(); break ;
            default:                    std::terminate();
        }
        setIOState(SEND_HTTP);
    } else {
        runCgiScript(reqInfo);
    }
    if (p_state != ERROR) {
        setPState(START_LINE); // reset state keep alive
    }
    return res;
}

off_t Client::getContentLength(std::string const& filename) const {
    struct stat statbuf;

    if (http_method == GET_METHOD && stat(filename.c_str(), &statbuf) == 0) {
        return statbuf.st_size;
    } else {
        return -1;
    }
}

// depends on request_uri being set
void Client::writeInitialPortion() {
    std::ostringstream oss;
    static const std::string conn = "Connection";

    oss << "HTTP/1.1 " << http_code << ' ' << getHttpStatus(http_code) << "\r\n";
    //oss << "Content-Type: " << getContentType(request_uri) << "\r\n";
    oss << "Content-Length: " << getContentLength(request_uri) << "\r\n";
    oss << conn << ": ";
    if (headers.count(conn)) {
        oss << headers[conn];
        //std::cout << "1-Connection: " << headers[conn];
    } else {
        oss << "close";
    }
    oss << "\r\n\r\n";
    filebuf = oss.str();
    send_it = filebuf.begin();
    send_ite = filebuf.end();
}

std::pair<std::string, std::string> Client::filterRequestUri() {
    size_t pos;
    struct stat statbuf;
    std::pair<std::string, std::string> reqInfo;

    pos = request_uri.find('?'); // get PATH_INFO
    if (pos != std::string::npos) {
        reqInfo.first = request_uri.substr(pos + 1);
        request_uri.erase(pos);
        //request_uri[pos] = '\0';
    }
    if (stat(request_uri.c_str(), &statbuf) == 0 && (statbuf.st_mode & S_IFMT) == S_IFDIR) {
        if (route && route->dir_list) {
            pos = request_uri.find('\0');
            if (pos == std::string::npos) {
                request_uri.append(route->dfl_file);
            } else {
                request_uri.insert(pos, route->dfl_file);
            }
        }
    }
//    if (route && !route->prefix_str.empty()) {
//        pos1 = request_uri.find(route->prefix_str);
//        if (pos1 != std::string::npos) {
//            request_uri.replace(0, pos1, route->root);
//        }
//    }
    if (route && !route->cgi_extension.empty()
        && (pos = request_uri.find(route->cgi_extension)) != std::string::npos) { // is cgi or not

        pos += route->cgi_extension.length();
        if (pos < request_uri.length()) {
            reqInfo.second = request_uri.substr(pos + 1); // get QUERY_STRING
        }
        if (reqInfo.second.empty()) {
            reqInfo.second = "/";
        } else {
            request_uri.erase(pos);
        }
        //request_uri[pos] = '\0';
    }
    return reqInfo;
}

int Client::performGetMethod() {

    if (!writeToFilebuf(request_uri)) {
        return -1;
    }
//    std::cout << "filebuf.length(): " << filebuf.length() << '\n';
//    std::cout << "filebuf:\n" << filebuf << '\n';
    send_it = filebuf.begin();
    send_ite = filebuf.end();
    return 0;
}

// Potential wastage here, as data is transferred from recvbuf to msgbody to outfile
int Client::performPostMethod() {
    std::ofstream outfile(request_uri.c_str(), std::ios_base::out | std::ios_base::binary);

    if (!outfile.is_open()) {
        perror(request_uri.c_str());
        setErrorState(4);
        return -1;
    }
    std::copy(msg_body.begin(), msg_body.end(), std::ostreambuf_iterator<char>(outfile));
    return 0;
}

int Client::performDeleteMethod() {
    const char *filename;

    filename = request_uri.c_str();
    if (access(filename, F_OK) == -1) {
        perror(filename);
        setErrorState(5);
        return -1;
    } /*else if (std::remove(filename) == -1) { // dont know if we can use this func from cstdio
        handle_error("std::remove");
        return -1;
    } else {
        return 0;
    } */
    return 0;
}

int Client::writeToFilebuf(std::string const& filename) {
    const char *filestr;
    std::ifstream infile;
    
    filestr = filename.c_str();
    //std::cout << "filestr: " << filestr << '\n';
    if (access(filestr, F_OK) == -1) {
        return setErrorState(404), 0;
    }
    infile.open(filestr);
    if (!infile.is_open()) {
        return setErrorState(403), 0;
    }
    std::copy((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>(), std::back_inserter(filebuf));
    return 1;
}
