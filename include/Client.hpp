#ifndef CLIENT_HPP
# define CLIENT_HPP

# include "ConfigFile.hpp"
# include "Server.hpp"
# include "Cgi.hpp"
# include <list>
# include <sys/types.h>
# include <sys/wait.h>
# include <sys/stat.h>
# include <sys/socket.h>
# include <sys/epoll.h>
# include <unistd.h>
# include <stdint.h>
# include <limits.h>

# define BUFSIZE 1024
# define TIMEOUT_VAL 5.0
# define handle_error(err) \
    do { std::cout << err << ": " << strerror(errno) << '\n'; exit(EXIT_FAILURE); } while (0);

namespace std {
    template<typename T, typename U>
    std::ostream& operator<<(std::ostream& os, std::pair<const T, U> const& obj) {
        return os << "first: " << obj.first << "\nsecond: "  << obj.second;
    }
    template<typename T>
    std::ostream& operator<<(std::ostream& os, std::pair<T, T> const& obj) {
        return os << obj.first << ", " << obj.second;
    }
}

template<typename T>
std::string str_convert(T const& value) {
    std::ostringstream oss;

    oss << value;
    return oss.str();
}

class CGI;

class Client {
    public:
        enum IOState {
            RECV_HTTP = 0,
            SEND_HTTP,
            RECV_CGI,
            SEND_CGI,
            CONN_CLOSED,
        };

        enum ParseState {
            ERROR = -1,
            START_LINE,
            HEADERS,
            MSG_BODY,
            FINISHED,
        };

        //Client();
        ~Client();
        //Client(Client const&);
        Client& operator=(Client const&);
        Client(int, int, std::vector<ServerInfo> const&);

        void socketRecv(); // could either be from client or cgi
        void socketSend();

        void closeFds();
        bool operator!=(int);
        bool operator==(int);
        bool isConnClosed() const;
        static int getEpollfd();
        static void setEpollfd(int);
        static void setEnvp(const char **);

        static std::string getHttpStatus(int);
        static off_t getContentLength(std::string const&);
        static std::string getContentType(std::string const&);

        static void addOpenFd(int);
        static void addOpenFds(const std::vector<int>&);
        static void closeOpenFds();
        static void deleteEvent(int);
        static void registerEvent(int, uint32_t);

        friend std::ostream& operator<<(std::ostream&, Client const&);

    private:
        void printFds() const;

        static std::map<int, std::string> initHttpStatuses();
        static std::map<std::string, std::string> initContentTypes();

        static int epollfd;
        static const char **envp;

        ServerInfo const& initServer(int, std::vector<ServerInfo> const&) const;

        void setPState(int);
        void setIOState(int);

        void setErrorState(int);

        void closeConnection();
        void resetSelf();

        void advanceSendIterators(size_t);

        void parseHttpRequest(const char *, size_t);
        RouteInfo const* findRouteInfo() const;
        int parseStartLine();
        int checkServerName();
        int parseHeaders(size_t&);
        int parseMsgBody(size_t);
        bool configureIOMethod(std::map<std::string, std::string> const&);
        int trackRecvBytes(size_t);
        int unchunkRequest();

        int performRequest();
        void writeInitialPortion();
        std::pair<std::string, std::string> filterRequestUri();
        int performGetMethod();
        int performPostMethod();
        int performDeleteMethod();
        int writeToFilebuf(std::string const&);

//        void parseCgiOutput(const char *, size_t);
//        int ignoreStartLine();
//        int parseCgiHeaders(size_t&);

        void runCgiScript(std::pair<std::string, std::string> const&);
        std::vector<char *> initCgiEnv(std::pair<std::string, std::string> const&) const;
        void executeCgi(int, std::pair<std::string, std::string> const&);

        int _pstate; // at which stage of parsing this object is currently in
        int _iostate; // should object be receiving/sending data at the moment
        int _httpcode;
        int _httpmethod;
        int _clientfd;

        time_t _lastresponsetime;

        bool _unchunkflag;
        bool _tracklength;
        size_t _bytesleft;

        std::string _recvbuf;
        std::string _filebuf;
        std::string _msgbody;
        std::string _requesturi;

        RouteInfo const* _route;
        ServerInfo const& _server;

        std::string::const_iterator _send_it;
        std::string::const_iterator _send_ite;

        std::list<CGI> _cgis;
        std::list<CGI>::iterator _currptr;

        std::map<std::string, std::string> _headers;

        static std::vector<int> open_fds;
};

#endif
