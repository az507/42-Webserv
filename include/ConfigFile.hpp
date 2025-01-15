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
    std::vector<std::string> cgi_extensions;
};

struct ServerInfo {
    static size_t max_bodysize;
    static std::map<int, std::string> error_pages;

    std::vector<int> connfds;
    std::vector<RouteInfo> routes;
    std::vector<std::string> names;
    std::vector<std::pair<std::string, std::string> > ip_addrs;
};

std::ostream& operator<<(std::ostream&, RouteInfo const&);
std::ostream& operator<<(std::ostream&, ServerInfo const&);

class ConfigFile {
    public:
        ConfigFile(const char *);
        void printServerInfo() const;
        std::vector<ServerInfo> const& getServerInfo() const;
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
        const std::vector<std::string> keywords;
        const std::vector<void (ConfigFile::*)(std::vector<std::string> const&, void *)> setters;

        void *convertIdxToAddr(int);
        void dirListHandler(std::vector<std::string> const&, void *);
        void httpMethodsHandler(std::vector<std::string> const&, void *);
        void errorPageHandler(std::vector<std::string> const&, void *);
        void maxBodysizeHandler(std::vector<std::string> const&, void *);
        void ipAddrsHandler(std::vector<std::string> const&, void *);
        void defaultStringHandler(std::vector<std::string> const&, void *);
        void defaultVectorHandler(std::vector<std::string> const&, void *);
        
        std::vector<std::string> initKeywords() const;
        std::vector<void (ConfigFile::*)(std::vector<std::string> const&, void *)> initSetters() const;

        ConfigFile();
        ConfigFile(ConfigFile const&);
        ConfigFile& operator=(ConfigFile const&);
};

#endif
