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
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>
#define MAXEVENTS 64
class Server
{
    public:
        Server(const std::vector<std::pair<std::string, std::string> > &serverinfo, const std::string &rootdir);  
        ~Server();
        void run();
    
    private:
        std::vector<int> serversockets;
        //int serversocket;
        int epollfd;
        std::string rootdir;
        //struct sockaddr_in serveraddress;
        //std::map<int, std::string> errorpage; // errorpage is a map defined in Server.hpp file error_pages[404] = "/errors/404.html"; error_pages[500] = "/errors/500.html";

        void initsocker(const std::vector<std::pair<std::string, std::string> > &serverinfo);
        void initepoll();
        void handleconnections();
        void handlerequest(int clientsocket);
        void severerrorpage(int clientsocket, int statuscode);
        void servestaticfile(int clientsocket, std::string filepath);
        void serverdirlisting(int clientsocket, std::string &dirpath);
        std::string sanitizepath(const std::string& basedir, const std::string& requestedpath);

        bool isdirectory(const std::string &path);
};

#endif
