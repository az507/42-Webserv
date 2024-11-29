#ifndef CLIENT_HPP
# define CLIENT_HPP

# include "ConfigFile.hpp"
# include <stdint.h>

# define BUFSIZE 1024

class Client {
    public:
        Client(int, std::vector<ServerInfo> const&);

        void socketRecv(); // could either be from client or cgi
        void socketSend();
        int getPState() const;
        int getIOState() const;

        bool operator==(int) const;

        static void setEnvp(char **);
        static void setEpollfd(int);
        static int getEpollfd();
    private:
        enum ParseState {
            ERROR = -1,
            START_LINE,
            HEADERS,
            MSG_BODY,
            FINISHED,
        };
        enum IOState {
            RECV_HTTP = 0,
            SEND_HTTP,
            RECV_CGI,
            SEND_CGI,
            CONN_CLOSED,
        };

        Client();
        ~Client();
        Client(Client const&);
        Client& operator=(Client const&);

        void setPState(int);
        void setIOState(int);

        void setFinishedState();

        std::map<int, std::string> initHttpStatuses() const;
        ServerInfo const& initServer(std::vector<ServerInfo> const&) const;

        static char **envp;
        static int epollfd;
        static const std::string delim;
        static const std::map<int, std::string> http_statuses;
        static const char *content_length, *transfer_encoding;

        RouteInfo const* findRoute() const;
        void writeInitialPortion(int);
        void configureIOMethod();

        void parseHttpRequest(const char *, size_t);
        bool parseStartLine(const char *, size_t);
        bool parseHeaders(const char *, size_t);
        bool parseMsgBody(const char *, size_t);
        bool trackRecvBytes(const char *, size_t);
        bool unchunkRequest(const char *, size_t);
        bool prepareResource();
        bool writeToFilebuf(std::string const&);

        void registerEvent(int);
        void executeCgi(int, std::string const&);

        int p_state; // at which stage of parsing this object is currently in
        int io_state; // should object be receiving/sending data at the moment
        int http_code;
        int http_method;
        int active_fd; // will be recv/send from this fd
        int passive_fd;

        bool config_flag;
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
