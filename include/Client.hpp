#ifndef CLIENT_HPP
# define CLIENT_HPP

# include "ConfigFile.hpp"
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/socket.h>
# include <sys/epoll.h>
# include <unistd.h>
# include <stdint.h>

# define BUFSIZE 1024
# define handle_error(err) \
    do { std::cout << err << ": " << strerror(errno) << '\n'; exit(EXIT_FAILURE); } while (0);

namespace std {
    template<typename T, typename U>
    std::ostream& operator<<(std::ostream& os, std::pair<const T, U> const& obj) {
        return os << "first: " << obj.first << "\nsecond: "  << obj.second;
    }
}

class Client {
    public:
        //Client();
        ~Client();
        //Client(Client const&);
        Client& operator=(Client const&);
        Client(int, std::vector<ServerInfo> const&);

        void socketRecv(); // could either be from client or cgi
        void socketSend();

        void closeFds();
        void setActiveFd(int);
        bool isConnClosed() const;
        bool operator!=(int) const;
        bool operator==(int) const;
        static int getEpollfd();
        static void setEpollfd(int);
        static void setEnvp(const char **);

        friend std::ostream& operator<<(std::ostream&, Client const&);

    private:
        static std::map<int, std::string> initHttpStatuses();
        static std::map<std::string, std::string> initContentTypes();

        std::string getHttpStatus(int) const;
        off_t getContentLength(std::string const&) const;
        std::string getContentType(std::string const&) const;

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

        static int epollfd;
        static const char **envp;

        ServerInfo const& initServer(int, std::vector<ServerInfo> const&) const;

        void setPState(int);
        void setIOState(int);

        void setErrorState(int);

        void closeConnection();
        void resetSelf();

        void advanceSendIterators(size_t);
        void deleteEvent(int);

        void parseHttpRequest(const char *, size_t);
        RouteInfo const* findRouteInfo() const;
        int parseStartLine();
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

        void parseCgiOutput(const char *, size_t);
        int ignoreStartLine();
        int parseCgiHeaders(size_t&);

        void runCgiScript(std::pair<std::string, std::string> const&);
        void executeCgi(int, std::pair<std::string, std::string> const&);
        void registerEvent(int, uint32_t);

        int p_state; // at which stage of parsing this object is currently in
        int io_state; // should object be receiving/sending data at the moment
        int http_code;
        int http_method;
        int active_fd; // will be recv/send from this fd
        int passive_fd;

        bool unchunk_flag;
        bool track_length;
        size_t bytes_left;

        std::string recvbuf;
        std::string filebuf;
        std::string msg_body;
        std::string request_uri;

        RouteInfo const* route;
        ServerInfo const& server;

        std::string::const_iterator send_it;
        std::string::const_iterator send_ite;

        std::map<std::string, std::string> headers;
};

#endif
