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
}