/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xzhang <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/12 10:31:17 by xzhang            #+#    #+#             */
/*   Updated: 2024/11/12 10:31:19 by xzhang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <sys/epoll.h>
#include <vector>
#include <fcntl.h>
#define MAXEVENTS 64
class Server
{
    public:
        Server(int port);
        ~Server();
        void run();
    
    private:
        int serversocket;
        int epollfd;
        struct sockaddr_in serveraddress;

        void initsocker(int port);
        void initepoll();
        void handleconnections();
        void handlerequest(int clientsocket);
};

#endif
