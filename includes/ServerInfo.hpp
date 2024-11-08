#ifndef SERVERINFO_HPP
# define SERVERINFO_HPP

# include "webserv.hpp"

enum DirectiveType {
    ADDR = 0,
};

struct Directives {
    std::string name;
    int nil_handler(ServerInfo&);
    void (*func)(ServerInfo&, std::vector<std::string> const&);
};

class RouteInfo {
    public:
        RouteInfo() :
            dirList(false),
            httpMethods(0) {}
        ~RouteInfo() {}

        bool dirList;
        int httpMethods;
        std::string root;
        std::string index;
        std::string redirect;
        std::string uploadDir;
        std::string fileExtension;

    private:
        RouteInfo(RouteInfo const&) {}
        void operator=(RouteInfo const&) {}
};

class ServerInfo {
    public:
        static int const maxClients;
        static std::map<int, std::string> errorPages;

        std::vector<int> sockfds;
        std::vector<RouteInfo> routes;
        std::vector<std::string> serverNames;

    private:
        ServerInfo(ServerInfo const&) {}
        void operator=(ServerInfo const&) {}
};

int ServerInfo::maxClients = 0;

#endif
