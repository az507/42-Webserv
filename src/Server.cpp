/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xzhang <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/12 15:52:25 by xzhang            #+#    #+#             */
/*   Updated: 2024/11/12 15:52:31 by xzhang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

Server::Server(int port)
{
    initsocker(port);
    initepoll();
    
}

Server::~Server()
{
    close(serversocket);
    close(epollfd);
}

void Server::initsocker(int port)
{
    serversocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serversocket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    int s = 1;
    if (setsockopt(serversocket, SOL_SOCKET, SO_REUSEADDR, &s, sizeof(s)) < 0)
    {
        perror("setsockpt failed");
        close(serversocket);
        exit(EXIT_FAILURE);
    }
    serveraddress.sin_family = AF_INET;
    serveraddress.sin_addr.s_addr = INADDR_ANY;
    serveraddress.sin_port = htons(port);

    if (bind(serversocket, (struct sockeaddr*)&serveraddress, sizeof(serveraddress)) < 0)
    {
        perror("Binding failed");
        close(serversocket);
        exit(EXIT_FAILURE);
    }
    if (listen(serversocket, 10) < 0)
    {
        perror("Listening failed");
        close(serversocket);
        exit(EXIT_FAILURE);
    }
    fcntl(serversocket, F_SETFL, O_NONBLOCK);
}

void Server::initepoll()
{
    epollfd = epoll_create(1);
    if (epollfd = -1)
    {
        perror("epoll creation failed");
        exit(EXIT_FAILURE);
    }
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = serversocket;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, serversocket, &ev) < 0)
    {
        perror("epoll_ctl failed");
        close(epollfd);
        close(serversocket);
        exit(EXIT_FAILURE);
    }
}

void Server::run()
{
    handleconnections();
}

void Server::handleconnections()
{
    struct epoll_event events[MAXEVENTS];
    while (1)
    {
        int eventcount = epoll_wait(epollfd, events, MAXEVENTS, -1);
        if (eventcount < 0)
        {
            perror("epoll_wait failed");
            break;
        }
        for (int i = 0; i < eventcount; ++i)
        {
            if (events[i].data.fd == serversocket)
            {
                int clientsocket = accept(serversocket, NULL, NULL);
                if (clientsocket >= 0)
                {
                    fcntl(clientsocket, F_SETFL, O_NONBLOCK);
                    struct epoll_event ev;
                    ev.events = EPOLLIN;
                    ev.data.fd = clientsocket;
                    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsocket, &ev) < 0)
                    {
                        perror("epoll_ctl failed");
                        close(clientsocket);
                    }
                }
            }
            else
            {
                handlerequest(events[i].data.fd);
            }
            if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL) < 0)
            {
                perror("epoll_ctl failed");
            }
            close(events[i].data.fd);
        }
    }
}


void Server::handlerequest(int clientsocket)
{
    char buffer[1024];
    int bytesread = read(clientsocket, buffer, 1024);
    if (bytesread < 0)
    {
        perror("read failed");
        close(clientsocket);
        return ;
    }
    if (bytesread == 0)
    {
        close(clientsocket);
        return ;
    }
    std::string request(buffer, bytesread);
    std::cout << request << std::endl;
    std::string response = "HTTP/1.1 200 OK\nContent-Length: 12\n\nHello world!";
    int byteswritten = write(clientsocket, response.c_str(), response.size());
    if (byteswritten != (int)response.size())
    {
        perror("write failed");
    }
    close();
}