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
    serversocket = socket(AF_INET, SOCK_STREAM, 0);// AF_INET: IPv4, SOCK_STREAM: TCP 0: default protocol return a file descriptor
    if (serversocket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    int s = 1;
    if (setsockopt(serversocket, SOL_SOCKET, SO_REUSEADDR, &s, sizeof(s)) < 0) // SOL_SOCKET: socket level, SO_REUSEADDR: reuse the address setsockopt: set the socket option reuse the address
    {
        perror("setsockpt failed");
        close(serversocket);
        exit(EXIT_FAILURE);
    }
    // serveraddress.sin_family = AF_INET;
    // serveraddress.sin_addr.s_addr = INADDR_ANY;  //
    // serveraddress.sin_port = htons(port);

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE; // bind to any address


    int status = getaddrinfo(NULL, "8080", &hints, &res);
    if (status != 0)
    {
        std::cerr << "getaddrinfo failed: " << gai_strerror(status) << std::endl;
        close(serversocket);
        exit(EXIT_FAILURE);
    }

    if (bind(serversocket, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("Binding failed");
        freeaddrinfo(res);
        close(serversocket);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);

    if (listen(serversocket, 10) < 0)
    {
        perror("Listening failed");
        close(serversocket);
        exit(EXIT_FAILURE);
    }




    // if (bind(serversocket, (struct sockeaddr*)&serveraddress, sizeof(serveraddress)) < 0)
    // {
    //     perror("Binding failed");
    //     close(serversocket);
    //     exit(EXIT_FAILURE);
    // }
    // if (listen(serversocket, 10) < 0)
    // {
    //     perror("Listening failed");
    //     close(serversocket);
    //     exit(EXIT_FAILURE);
    // }

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


void Server::handlerequest(int client_socket)
{
    char buffer[1024];
    int bytesreceived = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytesreceived > 0)
    {
        buffer[bytesreceived] = '\0';

        // Simple check for GET request
        std::string request(buffer);
        if (request.substr(0, 3) == "GET")
        {
            std::string response = "HTTP/1.1 200 OK\r\n"
                                   "Content-Type: text/html\r\n\r\n"
                                   "<html><body><h1>Hello, World!</h1></body></html>";
            send(client_socket, response.c_str(), response.size(), MSG_DONTWAIT);
        }
        // else if (bytesreceived == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
        // {
        //     return ;
        // } // Non-blocking socket, no data to read but can not  EAGAIN or EWOULDBLOCK
        else
        {
            perror("recv failed");
        }
    }
}