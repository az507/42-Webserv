#include "Client.hpp"

// return 0 to break out of outer do-while loop (to indicate ready to start sending msgs), non-zero for error
int Client::performRequest() {
    int res = 0;
    char *resolvedpath;
    std::pair<std::string, std::string> reqInfo;

    std::cout << "1) _requesturi: " << _requesturi << std::endl;
    reqInfo = filterRequestUri();
    writeInitialPortion();
    std::cout << "2) _requesturi: " << _requesturi << '\n' << std::endl;
    // sanitize path here
    //if (realpath(_requesturi.c_str(), resolvedpath)) {
    resolvedpath = canonicalize_file_name(_requesturi.c_str());
    if (resolvedpath) {
        _requesturi = resolvedpath;
        free(resolvedpath);
        resolvedpath = NULL;
        std::cout << "AFTER CANONICALIZE_FILE_NAME: _requesturi: " << _requesturi << std::endl;
    } else {
        //kill(getpid(), SIGSEGV);
        //perror(std::string(_requesturi).insert(0, "\tabc: ").c_str());
    }
    if (reqInfo.second.empty()) { // no cgi-extension found
        switch (_httpmethod) {
            case GET_METHOD:            res = performGetMethod(); break ;
            case POST_METHOD:           res = performPostMethod(); break ;
            case DELETE_METHOD:         res = performDeleteMethod(); break ;
            default:                    assert(0);
        }
        setIOState(SEND_HTTP);
    } else if (access(_requesturi.c_str(), F_OK | X_OK) == 0) {
        runCgiScript(reqInfo);
    }
    else {
        setErrorState(404);
        return(0);
    }
    if (_pstate != ERROR) {
        setPState(START_LINE); // reset state keep alive
    }
    return res;
}

off_t Client::getContentLength(std::string const& filename) {
    struct stat statbuf;

    std::cout << "\t>>> FILENAME IN GETCONTENTLENGTH() = " << filename << '$' << std::endl;
    if (/*_httpmethod == GET_METHOD && */stat(filename.c_str(), &statbuf) == 0) {
        return statbuf.st_size;
    } else {
        return -1;
    }
}

// depends on _requesturi being set
void Client::writeInitialPortion() {
    std::ostringstream oss;
    static const std::string conn = "Connection";

    oss << "HTTP/1.1 " << _httpcode << ' ' << getHttpStatus(_httpcode) << "\r\n";
    //oss << "Content-Type: " << getContentType(request_uri) << "\r\n";
    if (Client::_httpmethod == GET_METHOD || _pstate == ERROR)
        oss << "Content-Length: " << getContentLength(_requesturi) << "\r\n";
    else
        oss << "Content-Length: 0\r\n";
    oss << conn << ": ";
    if (_headers.count(conn)) {
        oss << _headers[conn];
        //std::cout << "1-Connection: " << _headers[conn];
    } else {
        oss << (_headers[conn] = "close");
    }
    oss << "\r\n\r\n";
    _filebuf = oss.str();
    _send_it = _filebuf.begin();
    _send_ite = _filebuf.end();
}

std::pair<std::string, std::string> Client::filterRequestUri() {
    size_t pos;
    struct stat statbuf;
    std::pair<std::string, std::string> reqInfo;
    std::vector<std::string>::const_iterator it, ite;

    pos = _requesturi.find('?'); // get PATH_INFO
    if (pos != std::string::npos) {
        reqInfo.first = _requesturi.substr(pos + 1);
        _requesturi.erase(pos);
        //_requesturi[pos] = '\0';
    }
    if (stat(_requesturi.c_str(), &statbuf) == 0 && (statbuf.st_mode & S_IFMT) == S_IFDIR) {
        if (_route && _route->dir_list) { // this should be to create html output of current directory
            ;
        } else if (_route && !_route->dfl_file.empty()) {
            if (*_requesturi.rbegin() != '/') {
                _requesturi.push_back('/');
            }
            pos = _requesturi.find('\0');
            if (pos == std::string::npos) {
                _requesturi.append(_route->dfl_file);
            } else {
                _requesturi.insert(pos, _route->dfl_file);
            }
        }
    }
//    if (_route && !_route->prefix_str.empty()) {
//        pos1 = _requesturi.find(_route->prefix_str);
//        if (pos1 != std::string::npos) {
//            _requesturi.replace(0, pos1, _route->root);
//        }
//    }
    if (_route && !_route->cgi_extensions.empty()) {
        ite = _route->cgi_extensions.end();
        for (std::vector<std::string>::const_iterator it = _route->cgi_extensions.begin(); it != ite; ++it) {
            if (!it->empty() && (pos = _requesturi.find(*it)) != std::string::npos) { // is cgi or not
                pos += it->length();
                if (pos < _requesturi.length()) {
                    reqInfo.second = _requesturi.substr(pos + 1); // get QUERY_STRING
                }
                if (reqInfo.second.empty()) {
                    reqInfo.second = "/";
                } else {
                    _requesturi.erase(pos);
                }
                break ;
            }
        }
    }
    return reqInfo;
}

std::string getBaseDir(const std::string& fullpath, const std::string& root) {
    std::string::const_iterator it;
    if (root == ".") {
        return root;
    }
    if (fullpath.size() > root.size()) {
        it = std::search(fullpath.begin(), fullpath.end(), root.begin(), root.end());
        if (it != fullpath.end()) {
            //it = std::find(it, fullpath.end(), '/');
            std::advance(it, root.size());
            if (std::distance(it, fullpath.end()) > 1) {
                return std::string(it + 1, fullpath.end());
            }
            return ".";
        }
    }
    return fullpath;
}

int Client::performGetMethod() {
    const char *oldpwd, *currpwd;
    std::string dirlist, basepath;

    if (_route && _route->dir_list && Server::isDirectory(_requesturi)) {
        currpwd = oldpwd = getenv("PWD");
        if (!oldpwd || chdir(_route->root.c_str()) == -1) {
            return perror("oldpwd || chdir"), setErrorState(500), -1;
        }
        //dirlist = Server::createDirListHtml(_requesturi);
        //dirlist = Server::createDirListHtml(getBaseDir(_requesturi, _route->root));
        if (_route->root != ".") {
            setenv("PWD", std::string(oldpwd).append(1, '/').append(_route->root).c_str(), 1);
            currpwd = getenv("PWD");
        }
        if (_requesturi == currpwd) {
            basepath = ".";
        } else {
            basepath = getBaseDir(_requesturi, currpwd);
        }
        dirlist = Server::createDirListHtml(basepath);
        assert(chdir(oldpwd) != -1);
        setenv("PWD", oldpwd, 1);
        std::cout << "\t===== DIRLIST =====" << std::endl;
        //std::cout << dirlist << std::endl;
        //throw std::exception();
        if (dirlist.empty()) {
            return setErrorState(500), -1; // generic server error
        } else {
            _filebuf = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: "
                + str_convert(dirlist.length()) + "\r\n\r\n" + dirlist;
        }
        std::cout << _filebuf << std::endl;
    } else if (!writeToFilebuf(_requesturi)) {
        return -1;
    }
//    std::cout << "_filebuf.length(): " << _filebuf.length() << '\n';
//    std::cout << "_filebuf:\n" << _filebuf << '\n';
    _send_it = _filebuf.begin();
    _send_ite = _filebuf.end();
    return 0;
}

// Potential wastage here, as data is transferred from _recvbuf to msgbody to outfile
// int Client::performPostMethod() {
//     std::ofstream outfile(_requesturi.c_str(), std::ios_base::out | std::ios_base::binary);

//     if (!outfile.is_open()) {
//         perror(_requesturi.c_str());
//         setErrorState(4);
//         return -1;
//     }
//     std::copy(_msgbody.begin(), _msgbody.end(), std::ostreambuf_iterator<char>(outfile));
//     return 0;
// }

int Client::performPostMethod() {
    std::ofstream outfile(_requesturi.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);

    if (!outfile.is_open()) {
        perror(_requesturi.c_str());
        setErrorState(403);
        return (1);
    }
    
    try {
        if (!_msgbody.empty()) {
            outfile.write(&_msgbody[0], _msgbody.size());
        }
        if (!outfile) {
            std::cerr << "Error writing to file: " << _requesturi << '\n';
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

    filename = _requesturi.c_str();
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
    std::copy((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>(), std::back_inserter(_filebuf));
    return 1;
}
