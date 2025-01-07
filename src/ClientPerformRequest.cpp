#include "Client.hpp"

// return 0 to break out of outer do-while loop (to indicate ready to start sending msgs), non-zero for error
int Client::performRequest() {
    int res = 0;
    char *resolvedpath;
    std::pair<std::string, std::string> reqInfo;

    std::cout << "1) request_uri: " << request_uri << std::endl;
    reqInfo = filterRequestUri();
    writeInitialPortion();
    std::cout << "2) request_uri: " << request_uri << '\n' << std::endl;
    // sanitize path here
    //if (realpath(request_uri.c_str(), resolvedpath)) {
    resolvedpath = canonicalize_file_name(request_uri.c_str());
    if (resolvedpath) {
        request_uri = resolvedpath;
        free(resolvedpath);
        resolvedpath = NULL;
        std::cout << "AFTER CANONICALIZE_FILE_NAME: request_uri: " << request_uri << std::endl;
    } else {
        //kill(getpid(), SIGSEGV);
        //perror(request_uri.c_str());
    }
    if (reqInfo.second.empty()) { // no cgi-extension found
        switch (http_method) {
            case GET_METHOD:            res = performGetMethod(); break ;
            case POST_METHOD:           res = performPostMethod(); break ;
            case DELETE_METHOD:         res = performDeleteMethod(); break ;
            default:                    std::terminate();
        }
        setIOState(SEND_HTTP);
    } else if (access(request_uri.c_str(), F_OK | X_OK) == 0) {
        runCgiScript(reqInfo);
    }
    else {
        setErrorState(404);
        return(0);
    }
    if (p_state != ERROR) {
        setPState(START_LINE); // reset state keep alive
    }
    return res;
}

off_t Client::getContentLength(std::string const& filename) const {
    struct stat statbuf;

    std::cout << "\t>>> FILENAME IN GETCONTENTLENGTH() = " << filename << '$' << std::endl;
    if (/*http_method == GET_METHOD && */stat(filename.c_str(), &statbuf) == 0) {
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
    if (Client::http_method == 1 || p_state == ERROR)
        oss << "Content-Length: " << getContentLength(request_uri) << "\r\n";
    oss << conn << ": ";
    if (headers.count(conn)) {
        oss << headers[conn];
        //std::cout << "1-Connection: " << headers[conn];
    } else {
        oss << (headers[conn] = "close");
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
    std::vector<std::string>::const_iterator it, ite;

    pos = request_uri.find('?'); // get PATH_INFO
    if (pos != std::string::npos) {
        reqInfo.first = request_uri.substr(pos + 1);
        request_uri.erase(pos);
        //request_uri[pos] = '\0';
    }
    if (stat(request_uri.c_str(), &statbuf) == 0 && (statbuf.st_mode & S_IFMT) == S_IFDIR) {
        if (route && route->dir_list) { // this should be to create html output of current directory
            ;
        } else if (route && !route->dfl_file.empty()) {
            if (*request_uri.rbegin() != '/') {
                request_uri.push_back('/');
            }
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
    if (route && !route->cgi_extensions.empty()) {
        ite = route->cgi_extensions.end();
        for (std::vector<std::string>::const_iterator it = route->cgi_extensions.begin(); it != ite; ++it) {
            if (!it->empty() && (pos = request_uri.find(*it)) != std::string::npos) { // is cgi or not
                pos += it->length();
                if (pos < request_uri.length()) {
                    reqInfo.second = request_uri.substr(pos + 1); // get QUERY_STRING
                }
                if (reqInfo.second.empty()) {
                    reqInfo.second = "/";
                } else {
                    request_uri.erase(pos);
                }
                break ;
            }
        }
    }
    return reqInfo;
}

int Client::performGetMethod() {
    std::string dirlist;

    if (route && route->dir_list && Server::isDirectory(request_uri)) {
        dirlist = Server::createDirListHtml(request_uri);
        std::cout << "\t===== DIRLIST =====" << std::endl;
        //std::cout << dirlist << std::endl;
        //throw std::exception();
        if (dirlist.empty()) {
            setErrorState(404);
        } else {
            filebuf = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: "
                + str_convert(dirlist.length()) + "\r\n\r\n" + dirlist;
        }
        std::cout << filebuf << std::endl;
    } else if (!writeToFilebuf(request_uri)) {
        return -1;
    }
//    std::cout << "filebuf.length(): " << filebuf.length() << '\n';
//    std::cout << "filebuf:\n" << filebuf << '\n';
    send_it = filebuf.begin();
    send_ite = filebuf.end();
    return 0;
}

// Potential wastage here, as data is transferred from recvbuf to msgbody to outfile
// int Client::performPostMethod() {
//     std::ofstream outfile(request_uri.c_str(), std::ios_base::out | std::ios_base::binary);

//     if (!outfile.is_open()) {
//         perror(request_uri.c_str());
//         setErrorState(4);
//         return -1;
//     }
//     std::copy(msg_body.begin(), msg_body.end(), std::ostreambuf_iterator<char>(outfile));
//     return 0;
// }

int Client::performPostMethod() {
    std::ofstream outfile(request_uri.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);

    if (!outfile.is_open()) {
        perror(request_uri.c_str());
        setErrorState(403);
        return (1);
    }
    
    try {
        if (!msg_body.empty()) {
            outfile.write(&msg_body[0], msg_body.size());
        }
        if (!outfile) {
            std::cerr << "Error writing to file: " << request_uri << '\n';
            setErrorState(500); //Internal sever error
            return -1;
        }
    } catch (std::exception const& e) {
        std::cerr << "Error writing to file: " << e.what() << '\n';
        setErrorState(500);//internal sever error
        return -1;
    }
    outfile.close();
    return 0;
}

int Client::performDeleteMethod() {
    const char *filename;

    filename = request_uri.c_str();
    if (access(filename, F_OK) == -1) {
        perror(filename);
        setErrorState(404);
        return -1;
    } else if (std::remove(filename) == -1) { // dont know if we can use this func from cstdio
        handle_error("std::remove");
        setErrorState(500);
        return -1;
    } else {
        return 0;
    } 
    return 0;
}

int Client::writeToFilebuf(std::string const& filename) {
    const char *filestr;
    std::ifstream infile;
    
    filestr = filename.c_str();
    //std::cout << "filestr: " << filestr << '\n';
    if (access(filestr, F_OK | R_OK) == -1 || Server::isDirectory(filename)) {
        perror(filestr);
        return setErrorState(404), 0;
    }
    infile.open(filestr);
    if (!infile.is_open()) {
        return setErrorState(403), 0;
    }
    std::copy((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>(), std::back_inserter(filebuf));
    return 1;
}
