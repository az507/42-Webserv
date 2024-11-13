#ifndef CONFIGFILE_HPP
# define CONFIGFILE_HPP

# include <map>
# include <string>
# include <vector>
# include <utility>
# include <fstream>
# include <sstream>
# include <iomanip>
# include <iostream>
# include <iterator>
# include <algorithm>
# include <stdexcept>
# include <functional>
# include <cstring>
# include <cassert>

# define GET_METHOD 1
# define POST_METHOD 2
# define DELETE_METHOD 4

struct RouteInfo {
    bool dir_list;
    int http_methods;
    std::string root;
    std::string dfl_file;
    std::string redirect;
    std::string upload_dir;
    std::string prefix_str;
    std::string cgi_extension;
};

struct ServerInfo {
    static int max_clients;
    static std::map<int, std::string> error_pages;

    std::vector<RouteInfo> routes;
    std::vector<std::string> names;
    std::vector<std::pair<std::string, std::string> > ip_addrs;
};

std::ostream& operator<<(std::ostream&, const RouteInfo&);
std::ostream& operator<<(std::ostream&, const ServerInfo&);

class ConfigFile {
    public:
        explicit ConfigFile(const char *);
        void printServerInfo() const;
        std::vector<ServerInfo> getServerInfo() const;
    private:
        enum TokenType {
            LISTEN = 0,
            SERVER_NAME,
            ERROR_PAGE,
            CLIENT_MAX,
            HTTP_METHODS,
            REDIRECT,
            ROOT,
            DIR_LIST,
            DFL_FILE,
            CGI_EXTENSION,
            UPLOAD_DIR,
            ERROR
        };

        std::ifstream infile;
        std::vector<ServerInfo> servers;
        std::vector<std::string> keywords;
        std::vector<void(ConfigFile::*)(const std::vector<std::string>&, void *)> setters;

        void *convertIdxToAddr(int);
        void dirListHandler(const std::vector<std::string>&, void *);
        void httpMethodsHandler(const std::vector<std::string>&, void *);
        void errorPageHandler(const std::vector<std::string>&, void *);
        void maxClientsHandler(const std::vector<std::string>&, void *);
        void ipAddrsHandler(const std::vector<std::string>&, void *);
        void defaultStringHandler(const std::vector<std::string>&, void *);
        void defaultVectorHandler(const std::vector<std::string>&, void *);
        
        std::vector<std::string> initKeywords() const;
        std::vector<void(ConfigFile::*)(const std::vector<std::string>&, void *)> initSetters() const;

        ConfigFile();
        ConfigFile(const ConfigFile&);
        ConfigFile& operator=(const ConfigFile&);
};

#endif
