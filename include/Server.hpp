/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xzhang <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/12 10:31:17 by xzhang            #+#    #+#             */
/*   Updated: 2024/11/15 12:23:19 by achak            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <sys/epoll.h>
#include <vector>
#include <fcntl.h>
#include <cstdio>
#include <map>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdlib.h>
#include <limits.h>
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
        //std::map<int, std::string> errorpage; // errorpage is a map defined in Server.hpp file error_pages[404] = "/errors/404.html"; error_pages[500] = "/errors/500.html";

        void initsocker(int port);
        void initepoll();
        void handleconnections();
        void handlerequest(int clientsocket);
        void severerrorpage(int clientsocket, int statuscode);
        void servestaticfile(int clientsocket, std::string filepath);
};

#endif
