#ifndef SOCKETBUFFER_HPP
# define SOCKETBUFFER_HPP

# define BUFSIZE 1024

class SocketBuffer {
    public:
        SocketBuffer(int);
        bool operator==(int) const;
        void recvData();
        void sendData();

        int getSockfd() const;
    private:
        int sockfd; // can be either from accept() or socketpair()
        std::string recvbuf;
        std::string sendbuf;
        std::ptrdiff_t recv_offset;
        std::ptrdiff_t send_offset;
};

class Client {
    public:
        Client(int, std::vector<ServerInfo> const&);

        bool operator==(int) const;
    private:
        void parseStartLine();
        void parseHeaders();
        void parseMsgBody();

        int http_code;
        int http_method;
        SocketBuffer s_buf;
        std::string http_msg;
        std::string request_uri;
        ServerInfo const& server;
        std::map<std::string, std::string> headers;
};

#endif
