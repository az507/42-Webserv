/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xzhang <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/12 10:30:39 by xzhang            #+#    #+#             */
/*   Updated: 2024/11/22 17:33:52 by achak            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

# include "ConfigFile.hpp"

# define BUFSIZE 1024

class Client {
    public:
        Client(int, std::vector<ServerInfo> const&);

        void socketRecv();
        void socketSend();
        void setSockFd(int);
        int getState() const;
        void setFilePath(std::string const&);
    private:
        enum StateType {
            ERROR = -1,
            START_LINE,
            HEADERS,
            MSG_BODY,
            SEND_READY,
            FINISHED,
        };

        void setFinishedState();
        ServerInfo const& initServer(std::vector<ServerInfo> const&) const;

        int state;
        int sockfd;
        std::string recvbuf;
        std::string sendbuf;
        std::string filepath;
        ServerInfo const& server;
        std::ptrdiff_t recv_offset;
        std::ptrdiff_t send_offset;

        int http_code;
        std::string http_method;
        std::string request_uri;
        std::map<std::string, std::string> headers;
};

#endif
