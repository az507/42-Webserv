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
        int getState() const;
        void setFilePath(std::string const&);

        bool operator==(int) const;
        static void setEpollfd(int);
        static int getEpollfd() const;
    private:
        enum StateType {
            ERROR = -1,
            NEED_MORE,
            START_LINE,
            HEADERS,
            MSG_BODY,
            RECV_DONE,
            SEND_READY,
            FINISHED,
        };

        void setFinishedState();
        ServerInfo const& initServer(std::vector<ServerInfo> const&) const;

        static int epollfd;
        static const std::string delim;
        static const std::map<int, std::string> http_statuses;
        static const char *content_length, *transfer_encoding;

        std::map<int, std::string> initHttpStatuses() const;

        int state;

        int active_fd; // will be recv from this fd
        int passive_fd;

        std::string recvbuf;
        std::string sendbuf;
        std::string filepath;
        RouteInfo const* route;
        ServerInfo const& server;
        std::ptrdiff_t recv_offset;
        std::ptrdiff_t send_offset;

        bool track_length;
        bool unchunk_flag;
        size_t bytes_left;

        int http_method;

        int http_code;
        std::string http_method;
        std::string request_uri;
        std::vector<unsigned char> msg_body;
        std::map<std::string, std::string> headers;
};

#endif
