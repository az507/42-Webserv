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
    (void)port;
    serversocket = socket(AF_INET, SOCK_STREAM, 0);// AF_INET: IPv4, SOCK_STREAM: TCP 0: default protocol return a file descriptor
    if (serversocket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server socket created: " << serversocket << std::endl;
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
        close(serversocket);
        freeaddrinfo(res);
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
    std::cout << "Server is listening on port " << port << std::endl;
}

void Server::initepoll()
{
    epollfd = epoll_create(1);
    if (epollfd == -1)
    {
        perror("epoll creation failed");
        exit(EXIT_FAILURE);
    }
    std::cout << "Epoll instance created: " << epollfd << std::endl;
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = serversocket;
    std::cout << "Adding server socket to epoll: " << serversocket << std::endl;
    std::cout << "Epoll FD: " << epollfd << std::endl;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, serversocket, &ev) < 0)
    {
        perror("epoll_ctl failed");
        close(epollfd);
        close(serversocket);
        exit(EXIT_FAILURE);
    }
    std::cout << "Server socket added to epoll successfully." << std::endl;

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
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = clientsocket;
                    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsocket, &ev) < 0)
                    {
                        perror("epoll_ctl failed1");
                        close(clientsocket);
                    }
                }
            }
            else
            {
                if (events[i].events & EPOLLIN)
                    handlerequest(events[i].data.fd);
                std::cout << "events[i].data.fd: " << events[i].data.fd << std::endl;
                close(events[i].data.fd);
                // if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL) < 0)
                // {
                //     perror("epoll_ctl failed2: "); 
                // }
            }

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

        std::string method = request.substr(0, request.find(" "));
        std::string url = request.substr(request.find(" ") + 1);//, request.find(" ", request.find(" ") + 1) - request.find(" ") - 1);
        url = url.substr(0, url.find(" "));
        std::cout << "Method: " << method << std::endl;
        std::cout << "URL: " << url << std::endl;

        if (method == "GET")
        {
            std::cout << "GET request received" << std::endl;
            std::string filepath = rootdir + url;
            if (filepath.back() == '/')
                filepath += "index.html";
            
            std::ifstream file(filepath.c_str());
            if (file)
            {
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                std::string response = "HTTP/1.1 200 OK\r\n"
                                       "Content-Type: text/html\r\n\r\n"
                                       "Content-Length: " + std::to_string(content.size()) + "\r\n\r\n"
                                       "<html><body>" + content + "</body></html>";
                send(client_socket, response.c_str(), response.size(), MSG_DONTWAIT);             
            }
            else
            {
                std::string error = "HTTP/1.1 404 Not Found\r\n"
                                    "Content-Type: text/html\r\n\r\n"
                                    "<html><body><h1>404 Not Found</h1></body></html>";
                send(client_socket, error.c_str(), error.size(), MSG_DONTWAIT);
            }
        }

        else if (method == "POST")
        {
            size_t start = request.find("\r\n\r\n") + 4; // start of the body
            std::string body = request.substr(start);

            std::ofstream outfile("upload.txt");
            outfile << body;
            outfile.close();

            std::string response = "HTTP/1.1 200 OK\r\n"
                                   "Content-Type: text/html\r\n\r\n"
                                   "<html><body><h1>File uploaded successfully</h1></body></html>";
            send(client_socket, response.c_str(), response.size(), MSG_DONTWAIT);

        }

        else if (method == "DELETE")
        {
            std::string filepath = rootdir + url;
            if (remove(filepath.c_str()) == 0)
            {
                std::string response = "HTTP/1.1 200 OK\r\n"
                                       "Content-Type: text/html\r\n\r\n"
                                       "<html><body><h1>File deleted successfully</h1></body></html>";
                send(client_socket, response.c_str(), response.size(), MSG_DONTWAIT);
            }
            else
            {
                severerrorpage(client_socket, 404);
            }
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
    close(client_socket);
}



// void Server::handlerequest(int client_socket)
// {
//     char buffer[1024];
//     int bytesreceived = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

//     if (bytesreceived > 0)
//     {
//         buffer[bytesreceived] = '\0';

//         // Simple check for GET request 
//         std::string request(buffer);
//         if (request.substr(0, 3) == "GET")
//         {
//             std::string response = "HTTP/1.1 200 OK\r\n"
//                                    "Content-Type: text/html\r\n\r\n"
//                                    //"connection: close\r\n\r\n"
//                                    "<html><body><h1>Hello, World!</h1></body></html>";
//             send(client_socket, response.c_str(), response.size(), MSG_DONTWAIT);
//         }
//         // else if (bytesreceived == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
//         // {
//         //     return ;
//         // } // Non-blocking socket, no data to read but can not  EAGAIN or EWOULDBLOCK
//         else
//         {
//             perror("recv failed");
//         }
//     }
//     close(client_socket);
// }

void Server::severerrorpage(int clientsocket, int statuscode)
{
    std::string statusmessage;
    switch (statuscode)
    {
        case 404:
            statusmessage = "Not Found";
            break;
        case 500:
            statusmessage = "Internal Server Error";
            break;
        default:
            statusmessage = "Not Found";
            break;
        
        std::string errorpagepayth = errorpage[statuscode];
        std::ifstream file(errorpagepath.c_str());
        std::string response;

        if (feile)
        {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            response = "HTTP/1.1 " + std::to_string(statuscode) + " " + statusmessage + "\r\n"
                       "Content-Type: text/html\r\n\r\n"
                       "Content-Length: " + std::to_string(content.size()) + "\r\n\r\n"
                       "<html><body>" + content + "</body></html>";
        }
        else
        {
            response = "HTTP/1.1 " + std::to_string(statuscode) + " " + statusmessage + "\r\n"
                       "Content-Type: text/html\r\n\r\n"
                       "<html><body><h1>" + std::to_string(statuscode) + " " + statusmessage + "</h1></body></html>";
        }
        send(clientsocket, response.c_str(), response.size(), MSG_DONTWAIT);
    }
}