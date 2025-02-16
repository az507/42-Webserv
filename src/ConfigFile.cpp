#include "ConfigFile.hpp"

size_t ServerInfo::max_bodysize = 0;
std::map<int, std::string> ServerInfo::error_pages;

/*
std::string getBasename(const std::string& fullpath) {
    size_t pos;
    if (fullpath.size() > 1) {
        pos = fullpath.find_last_of('/', fullpath[fullpath.size() - 1] == '/' ? fullpath.length() - 2 : std::string::npos);
        if (pos != std::string::npos && ++pos < fullpath.length()) {
            return fullpath.substr(pos, fullpath.find('/', pos));
        }
    }
    return fullpath;
}
    char *path = NULL;
    for (std::vector<ServerInfo>::iterator it = servers.begin(); it != servers.end(); ++it) {
        for (std::vector<RouteInfo>::iterator r_it = it->routes.begin(); r_it != it->routes.end(); ++r_it) {
            if (!r_it->root.empty()) {
                path = canonicalize_file_name(r_it->root.c_str());
                if (!path) {
                    const char *err = strerror(errno);
                    throw std::runtime_error(std::string("canonicalize_file_name: ") + err);
                }
                r_it->root = getBasename(path);
                free(path);
            }
        }
    }
*/

ConfigFile::ConfigFile(const char *filename) : infile(filename), keywords(initKeywords()), setters(initSetters()) {
    size_t pos;
    int token_type;
    std::stringstream ss;
    std::string line, token;
    std::vector<std::string> values;

    if (!infile.is_open()) {
        throw std::invalid_argument(std::string(filename) + ": " + strerror(errno));
    }
    while (!getline(infile, line).eof()) {
        if (!infile) {
            infile.exceptions(infile.rdstate());
        } else if (!line.empty() && std::find_if(line.begin(), line.end(), std::not1(std::ptr_fun(&isspace))) != line.end()) {
            //std::unary_negate<std::pointer_to_unary_function<int, int> >(std::ptr_fun(&isspace))) != line.end()) {

            pos = line.find('=');
            if (pos == std::string::npos) {
                line.erase(std::remove_if(line.begin(), line.end(), &isspace), line.end());
                if (line == "server") {
                    std::cout << "\t in here, creating another server block\n";
                    servers.push_back((ServerInfo){});
                } else if (!line.compare(0, 8, "location")) {
                    servers.at(servers.size() - 1).routes.push_back((RouteInfo){}); //std::vector<>::at should throw exception if server.empty()
                    servers.back().routes.back().prefix_str = line.substr(8);
                } else {
                    throw std::runtime_error(std::string("Unexpected token: ") + line);
                }
            } else {
                line.erase(std::remove_if(line.begin(), line.begin() + pos, &isspace), line.begin() + pos);
                token = line.substr(0, pos = line.find('='));
                ss.str(line.c_str() + pos + 1);
                token_type = std::find_if(keywords.begin(), keywords.end(), std::bind2nd(std::equal_to<std::string>(), token)) - keywords.begin();
                if (token_type == ERROR) {
                    throw std::runtime_error(std::string("invalid token: ") + token);
                } else if (servers.empty()) {
                    throw std::runtime_error("Declare a server context first");
                } else if (token_type >= HTTP_METHODS && servers.back().routes.empty()) {
                    throw std::runtime_error("Declare a location context first");
                }
                std::copy(std::istream_iterator<std::string>(ss), std::istream_iterator<std::string>(), std::back_inserter(values));
                (this->*setters[token_type])(values, convertIdxToAddr(token_type));
                values.clear();
                ss.clear();
            }
        }
    }
    printServerInfo();
    //throw std::exception();
}

std::ostream& operator<<(std::ostream& os, RouteInfo const& route) {

    os << std::setw(20) << "===RouteInfo===\n";
    os << std::setw(20) << "dir_list: " << std::boolalpha << route.dir_list << '\n';
    os << std::setw(20) << "http_methods number val: " << route.http_methods << '\n';
    os << std::setw(20) << "http_methods: ";
    if (route.http_methods & GET_METHOD)
        os << "GET, ";
    if (route.http_methods & POST_METHOD)
        os << "POST, ";
    if (route.http_methods & DELETE_METHOD)
        os << "DELETE";
    os.put('\n');
    os << std::setw(20) << "root: " << route.root << '\n';
    os << std::setw(20) << "dfl_file: " << route.dfl_file << '\n';
    os << std::setw(20) << "upload_dir: " << route.upload_dir << '\n';
    os << std::setw(20) << "prefix_str: " << route.prefix_str << '\n';
    os << std::setw(20) << "cgi_extensions: ";
    std::copy(route.cgi_extensions.begin(), route.cgi_extensions.end(), std::ostream_iterator<std::string>(os, ", "));
    return os << '\n';
}

std::ostream& operator<<(std::ostream& os, ServerInfo const& serv) {
    std::ios oldstate(std::cout.rdbuf());

    os << "===ServerInfo===\nserver_names: ";
    std::copy(serv.names.begin(), serv.names.end(), std::ostream_iterator<std::string>(os, ", "));
    os << "\nIp-addresses: ";
    for (std::vector<std::pair<std::string, std::string> >::const_iterator it = serv.ip_addrs.begin(); it != serv.ip_addrs.end(); ++it) {
        os << "-->" << it->first << ':' << it->second << '\n' << std::setw(17);
    }
//    os << "Connfds: ";
//    *std::copy(serv.connfds.begin(), serv.connfds.end(), std::ostream_iterator<int>(os, ", ")) = '\n';
    std::copy(serv.routes.begin(), serv.routes.end(), std::ostream_iterator<RouteInfo>(os, "\n"));
    os.copyfmt(oldstate);
    return os;
}

void ConfigFile::printServerInfo() const {

    std::cout << "max_bodysize: " << ServerInfo::max_bodysize << '\n';
    std::cout << "error_pages: " << std::setw(4);
    for (std::map<int, std::string>::const_iterator it = ServerInfo::error_pages.begin(); it != ServerInfo::error_pages.end(); ++it) {
        std::cout << it->first << " => " << it->second << '\n' << std::setw(17);
    }
    std::copy(servers.begin(), servers.end(), std::ostream_iterator<ServerInfo>(std::cout << '\n'));
}

std::vector<ServerInfo> const& ConfigFile::getServerInfo() const {

    return servers;
}

void *ConfigFile::convertIdxToAddr(int idx) {

    switch (idx) {
        case LISTEN:        return reinterpret_cast<void *>(&servers.back().ip_addrs);
        case SERVER_NAME:   return reinterpret_cast<void *>(&servers.back().names);
        case ERROR_PAGE:    return reinterpret_cast<void *>(&ServerInfo::error_pages);
        case CLIENT_MAX:    return reinterpret_cast<void *>(&ServerInfo::max_bodysize);
        case HTTP_METHODS:  return reinterpret_cast<void *>(&servers.back().routes.back().http_methods);
        case REDIRECT:      return reinterpret_cast<void *>(&servers.back().routes.back().redirect);
        case ROOT:          return reinterpret_cast<void *>(&servers.back().routes.back().root);
        case DIR_LIST:      return reinterpret_cast<void *>(&servers.back().routes.back().dir_list);
        case DFL_FILE:      return reinterpret_cast<void *>(&servers.back().routes.back().dfl_file);
        case CGI_EXTENSION: return reinterpret_cast<void *>(&servers.back().routes.back().cgi_extensions);
        case UPLOAD_DIR:    return reinterpret_cast<void *>(&servers.back().routes.back().upload_dir);
        default:            assert(0);
    }
    return NULL;
}

void ConfigFile::dirListHandler(std::vector<std::string> const& values, void *addr) {

    *(reinterpret_cast<bool *>(addr)) = values.back() == "on" ? true : false;
}

void ConfigFile::httpMethodsHandler(std::vector<std::string> const& values, void *addr) {
    int *ptr;
    std::vector<std::string>::const_iterator ite = values.end();

    *(ptr = reinterpret_cast<int *>(addr)) = 0;
    for (std::vector<std::string>::const_iterator it = values.begin(); it != ite; ++it) {
        if (*it == "GET") {
            *ptr |= GET_METHOD;
        } else if (*it == "POST") {
            *ptr |= POST_METHOD;
        } else if (*it == "DELETE") {
            *ptr |= DELETE_METHOD;
        }
    }
}

void ConfigFile::errorPageHandler(std::vector<std::string> const& values, void *addr) {
    int error_nbr;
    std::map<int, std::string> *ptr;
    std::vector<std::string>::const_iterator p;

    if (values.size() < 2) {
        throw std::invalid_argument("not enough arguments for error_page directive");
    }
    ptr = reinterpret_cast<std::map<int, std::string> *>(addr);
    for (std::vector<std::string>::const_iterator it = values.begin(); it != values.end(); ++it) {
        for (p = it; p != values.end(); ++p) {
            if (p->find('/') != std::string::npos) {
                break ;
            }
        }
        for ( ; it != p; ++it) {
            error_nbr = atoi(it->c_str());
            if (!ptr->count(error_nbr)) {
                ptr->operator[](error_nbr) = *p;
            } else {
                std::cerr << error_nbr << " already exists in error_pages map\n";
            }
        }
    }
}

void ConfigFile::maxBodysizeHandler(std::vector<std::string> const& values, void *addr) {
    if (!(std::istringstream(values.back()) >> *(reinterpret_cast<size_t*>(addr)))) {
        throw std::runtime_error("invalid conversion for max_bodysize");
    }
    //*(reinterpret_cast<size_t *>(addr)) = atoi(values.back().c_str());
}

void ConfigFile::ipAddrsHandler(std::vector<std::string> const& values, void *addr) {
    size_t findpos1, findpos2;
    std::vector<std::pair<std::string, std::string> > *ptr;
    std::vector<std::string>::const_iterator ite = values.end();

    ptr = reinterpret_cast<std::vector<std::pair<std::string, std::string> > *>(addr);
    for (std::vector<std::string>::const_iterator it = values.begin(); it != ite; ++it) {
        findpos1 = it->find(':');
        findpos2 = it->find('.');
        if (findpos1 == std::string::npos) {
            if (findpos2 == std::string::npos) {
                ptr->push_back(std::make_pair("", *it));
            } else {
                ptr->push_back(std::make_pair(*it, ""));
            }
        } else {
            ptr->push_back(std::make_pair(it->substr(0, findpos1), it->substr(findpos1 + 1)));
        }
    }
}

void ConfigFile::defaultStringHandler(std::vector<std::string> const& values, void *addr) {

    *(reinterpret_cast<std::string *>(addr)) = values.back();
}

void ConfigFile::defaultVectorHandler(std::vector<std::string> const& values, void *addr) {

    std::copy(values.begin(), values.end(), std::back_inserter(*(reinterpret_cast<std::vector<std::string> *>(addr))));
}

std::vector<void(ConfigFile::*)(std::vector<std::string> const&, void *)> ConfigFile::initSetters() const {
    std::vector<void(ConfigFile::*)(std::vector<std::string> const&, void *)> setters(11);

    setters[0] = &ConfigFile::ipAddrsHandler;
    setters[1] = &ConfigFile::defaultVectorHandler;
    setters[2] = &ConfigFile::errorPageHandler;
    setters[3] = &ConfigFile::maxBodysizeHandler;
    setters[4] = &ConfigFile::httpMethodsHandler;
    setters[5] = &ConfigFile::defaultStringHandler;
    setters[6] = &ConfigFile::defaultStringHandler;
    setters[7] = &ConfigFile::dirListHandler;
    setters[8] = &ConfigFile::defaultStringHandler;
    setters[9] = &ConfigFile::defaultVectorHandler;
    setters[10] = &ConfigFile::defaultStringHandler;
    return setters;
}

std::vector<std::string> ConfigFile::initKeywords() const {
    std::vector<std::string> keywords(11);

    keywords[0] = "listen";
    keywords[1] = "server_name";
    keywords[2] = "error_page";
    keywords[3] = "client_max_body_size";
    keywords[4] = "http_methods";
    keywords[5] = "return";
    keywords[6] = "root";
    keywords[7] = "autoindex";
    keywords[8] = "index";
    keywords[9] = "cgi_extension";
    keywords[10] = "upload_store";
    return keywords;
}

ConfigFile::ConfigFile() {}
ConfigFile::ConfigFile(ConfigFile const&) {}
ConfigFile& ConfigFile::operator=(ConfigFile const&) { return *this; }
