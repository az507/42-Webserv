#ifndef CLIENT_HPP
# define CLIENT_HPP

# include "ConfigFile.hpp"
# include <stdint.h>

# define BUFSIZE 1024

class Client {
    public:
        Client(int, std::vector<ServerInfo> const&);

        void socketRecv(int); // could either be from client or cgi
        void socketSend();
        void setSockFd(int);
        int getPState() const;
        int getIOState() const;

        bool operator==(int) const;
        static void setEnvp(char **);
        static void setEpollfd(int);
        static int getEpollfd();
    private:
        enum ParseState {
            ERROR = -1,
            NEED_MORE,
            START_LINE,
            HEADERS,
            MSG_BODY,
            FINISHED,
        };
        enum IOState {
            RECV_HTTP,
            SEND_HTTP,
            RECV_CGI,
            SEND_CGI,
            CONN_CLOSED,
        };

        int p_state; // at which stage of parsing this object is currently in
        int io_state; // should object be receiving/sending data at the moment

        Client();
        ~Client();
        Client(Client const&);
        Client& operator=(Client const&);

        void setPState(int);
        void setIOState(int);

        void setFinishedState();
        ServerInfo const& initServer(std::vector<ServerInfo> const&) const;

        static char **envp;
        static int epollfd;
        static const std::string delim;
        static const std::map<int, std::string> http_statuses;
        static const char *content_length, *transfer_encoding;

        std::map<int, std::string> initHttpStatuses() const;

        int active_fd; // will be recv/send from this fd
        int passive_fd;

        // registerEvent()
        // executeCgi()

        std::string recvbuf;
        std::string sendbuf;
        RouteInfo const* route;
        ServerInfo const& server;

        std::string::const_iterator send_it;
        std::string::const_iterator send_ite;

        bool track_length;
        bool unchunk_flag;
        bool config_flag;
        size_t bytes_left;

        int http_method;

        int http_code;
        std::string request_uri;
        std::string msg_body;
        std::map<std::string, std::string> headers;
};

#endif
