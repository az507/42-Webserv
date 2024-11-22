#ifndef CLIENTINFO_HPP
# define CLIENTINFO_HPP

# include <map>
# include <iostream>
# include <sstream>
# include <sys/types.h>
# include <sys/socket.h>
# include "ConfigFile.hpp"

# define BUFSIZE 1024

class ClientInfo {
    public:
        //int checkTimeout();
        explicit ClientInfo(int);
        int readDataFrom();
        //int sendDataTo();

        //ClientInfo(ClientInfo const&);
        friend std::ostream& operator<<(std::ostream&, ClientInfo const&);
    private:
        enum InfoType {
            START_LINE = 0,
            HEADER,
            MSG_BODY,
        };
        enum ParseResult {
            NEED_MORE = 0,
            SUCCESS,
            ERROR,
        };

        int parse(std::string const&);
        int parseMsgBody(std::string const&);
        int parseHeaders(std::string const&);
        int parseStartLine(std::string const&);

        //ClientInfo();
        //ClientInfo& operator=(ClientInfo const&);

        int sockfd;
        int info_type;
        int bytes_left;
        int http_method;
        std::string sendbuf;
        std::string recvbuf;
        std::string msg_body;
        std::string start_line;
        std::string request_uri;
        std::map<std::string, std::string> headers;
};

#endif
