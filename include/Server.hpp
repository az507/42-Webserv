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
#include <poll.h>
#include <vector>

class Server
{
    public:
        Server(int port);
        ~Server();
        void run();
    
    private:
        int serversocket;
        std::vector<struct pollfd> connections;
        struct sockaddr_in address;

        void initsocker(int port);
        void handleconnections();
        void handlerequest(int clientsocket);
};

#endif
