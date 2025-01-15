/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xzhang <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/12 15:52:25 by xzhang            #+#    #+#             */
/*   Updated: 2025/01/13 17:18:59 by achak            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

Server::Server(const std::vector<std::pair<std::string, std::string> > &serverinfo, const std::string &rootdir) :rootdir(rootdir)
{
    initsocker(serverinfo);
    errorpage.insert(std::make_pair(404, "/errors/404.html"));
    errorpage.insert(std::make_pair(500, "/errors/500.html"));
    initepoll();

    
}

Server::~Server()
{
    for (size_t i = 0; i < serversockets.size(); ++i)
    {
        close(serversockets[i]);
    }

    //close(serversocket);
    close(epollfd);
}

void Server::initsocker(const std::vector<std::pair<std::string, std::string> > &serverinfo)
{
    for (size_t i = 0; i < serverinfo.size(); ++i)
    {
        std::string ip = serverinfo[i].first;
        std::string port = serverinfo[i].second;
        
        int serversocket;
        serversocket = socket(AF_INET, SOCK_STREAM, 0);// AF_INET: IPv4, SOCK_STREAM: TCP 0: default protocol return a file descriptor
        if (serversocket < 0)                           
        {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }
        std::cout << "Server socket created: " << serversocket << std::endl;
        int s = 1;
        // if (setsockopt(serversocket, SOL_SOCKET, SO_REUSEADDR, &s, sizeof(s)) < 0) // SOL_SOCKET: socket level, SO_REUSEADDR: reuse the address setsockopt: set the socket option reuse the address
        // {
        //     perror("setsockpt failed");
        //     close(serversocket);
        //     exit(EXIT_FAILURE);
        // }
        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET; // IPv4
        hints.ai_socktype = SOCK_STREAM; // TCP
        hints.ai_flags = AI_PASSIVE; // bind to any address

         int status = getaddrinfo(ip.empty() ? NULL : ip.c_str(), port.c_str(), &hints, &res);
        //int status = getaddrinfo(NULL, "8080", &hints, &res);
        if (status != 0)
        {
              std::cerr << "getaddrinfo failed for " << ip << ":" << port
                      << ": " << gai_strerror(status) << std::endl;
            //std::cerr << "getaddrinfo failed: " << gai_strerror(status) << std::endl;
            //close(serversocket);
            exit(EXIT_FAILURE);
        }
        if (setsockopt(serversocket, SOL_SOCKET, SO_REUSEADDR, &s, sizeof(s)) < 0) // SOL_SOCKET: socket level, SO_REUSEADDR: reuse the address setsockopt: set the socket option reuse the address
        {
            perror("setsockpt failed");
            close(serversocket);
            freeaddrinfo(res);
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
        serversockets.push_back(serversocket);
        //std::cout << "Server is listening on port " << serverinfo[i].second << std::endl;
                std::cout << "Server is listening on " << (ip.empty() ? "all interfaces" : ip)
                  << ":" << port << std::endl;
    }
}
//{
//     (void)port;
//     serversocket = socket(AF_INET, SOCK_STREAM, 0);// AF_INET: IPv4, SOCK_STREAM: TCP 0: default protocol return a file descriptor
//     if (serversocket < 0)
//     {
//         perror("Socket creation failed");
//         exit(EXIT_FAILURE);
//     }
//     std::cout << "Server socket created: " << serversocket << std::endl;
//     int s = 1;
//     if (setsockopt(serversocket, SOL_SOCKET, SO_REUSEADDR, &s, sizeof(s)) < 0) // SOL_SOCKET: socket level, SO_REUSEADDR: reuse the address setsockopt: set the socket option reuse the address
//     {
//         perror("setsockpt failed");
//         close(serversocket);
//         exit(EXIT_FAILURE);
//     }
//     // serveraddress.sin_family = AF_INET;
//     // serveraddress.sin_addr.s_addr = INADDR_ANY;  //
//     // serveraddress.sin_port = htons(port);

//     struct addrinfo hints, *res;
//     memset(&hints, 0, sizeof(hints));
//     hints.ai_family = AF_INET; // IPv4
//     hints.ai_socktype = SOCK_STREAM; // TCP
//     hints.ai_flags = AI_PASSIVE; // bind to any address


//     int status = getaddrinfo(NULL, "8080", &hints, &res);
//     if (status != 0)
//     {
//         std::cerr << "getaddrinfo failed: " << gai_strerror(status) << std::endl;
//         close(serversocket);
//         exit(EXIT_FAILURE);
//     }

//     if (bind(serversocket, res->ai_addr, res->ai_addrlen) < 0)
//     {
//         perror("Binding failed");
//         close(serversocket);
//         freeaddrinfo(res);
//         exit(EXIT_FAILURE);
//     }

//     freeaddrinfo(res);

//     if (listen(serversocket, 10) < 0)
//     {
//         perror("Listening failed");
//         close(serversocket);
//         exit(EXIT_FAILURE);
//     }

//     // if (bind(serversocket, (struct sockeaddr*)&serveraddress, sizeof(serveraddress)) < 0)
//     // {
//     //     perror("Binding failed");
//     //     close(serversocket);
//     //     exit(EXhttp://localhost:8080/subdir/test.htmlIT_FAILURE);
//     // }
//     // if (listen(serversocket, 10) < 0)
//     // {
//     //     perror("Listening failed");
//     //     close(serversocket);
//     //     exit(EXIT_FAILURE);
//     // }
//     std::cout << "Server is listening on port " << port << std::endl;
// }

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
    for (size_t i = 0; i < serversockets.size(); ++i)
    {
        ev.data.fd = serversockets[i];
        std::cout << "Adding server socket to epoll: " << serversockets[i] << std::endl;
        std::cout << "Epoll FD: " << epollfd << std::endl;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, serversockets[i], &ev) < 0)
        {
            perror("epoll_ctl failed");
            close(epollfd);
            close(serversockets[i]);
            exit(EXIT_FAILURE);
        }
        std::cout << "Server socket added to epoll successfully." << std::endl;
    }
}

// void Server::initepoll()
// {
//     epollfd = epoll_create(1);
//     if (epollfd == -1)
//     {
//         perror("epoll creation failed");
//         exit(EXIT_FAILURE);
//     }
//     std::cout << "Epoll instance created: " << epollfd << std::endl;
//     struct epoll_event ev;
//     ev.events = EPOLLIN;
//     ev.data.fd = serversocket;
//     std::cout << "Adding server socket to epoll: " << serversocket << std::endl;
//     std::cout << "Epoll FD: " << epollfd << std::endl;
//     if (epoll_ctl(epollfd, EPOLL_CTL_ADD, serversocket, &ev) < 0)
//     {
//         perror("epoll_ctl failed");
//         close(epollfd);
//         close(serversocket);
//         exit(EXIT_FAILURE);
//     }
//     std::cout << "Server socket added to epoll successfully." << std::endl;

// }

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
            int fd = events[i].data.fd;
            //if (events[i].data.fd == serversockets[i])
            //{
                if (std::find(serversockets.begin(), serversockets.end(), fd) != serversockets.end()) // check if the server socket is in the list of server sockets
                {
                    int clientsocket = accept(fd, NULL, NULL);
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
            //}
            else
            {
                if (events[i].events & EPOLLIN)
                    handlerequest(fd);
                std::cout << "events[i].data.fd: " << events[i].data.fd << std::endl;
                std::cout << "fd: " << fd << std::endl;
                close(fd);
                // if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL) < 0)
                // {
                //     perror("epoll_ctl failed2: "); 
                // }
            }
        }
    }
}

// void Server::handleconnections()
// {
//     struct epoll_event events[MAXEVENTS];
//     while (1)
//     {
//         int eventcount = epoll_wait(epollfd, events, MAXEVENTS, -1);
//         if (eventcount < 0)
//         {
//             perror("epoll_wait failed");
//             break;
//         }
//         for (int i = 0; i < eventcount; ++i)
//         {
//             if (events[i].data.fd == serversocket)
//             {
//                 int clientsocket = accept(serversocket, NULL, NULL);
//                 if (clientsoserverdirlistingcket >= 0)
//                 {
//                     struct epoll_event ev;
//                     ev.events = EPOLLIN | EPOLLET;
//                     ev.data.fd = clientsocket;
//                     if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsocket, &ev) < 0)
//                     {
//                         perror("epoll_ctl failed1");
//                         close(clientsocket);
//                     }
//                 }
//             }
//             else
//             {
//                 if (events[i].events & EPOLLIN)
//                     handlerequest(events[i].data.fd);
//                 std::cout << "events[i].data.fd: " << events[i].data.fd << std::endl;
//                 close(events[i].data.fd);
//                 // if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL) < 0)
//                 // {
//                 //     perror("epoll_ctl failed2: "); 
//                 // }
//             }

//         }
//     }
// }


void Server::servestaticfile(int clientsocket, std::string filepath)
{
    std::ifstream file(filepath.c_str());
    if (file)
    {
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        std::ostringstream oss;
        oss << content.size();

        std::string response = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/html\r\n\r\n"
                               "Content-Length: " + oss.str() + "\r\n\r\n"
                               "<html><body>" + content + "</body></html>";
        send(clientsocket, response.c_str(), response.size(), MSG_DONTWAIT);
    }
    else
    {
        severerrorpage(clientsocket, 404);
    }
}    

/*
std::string Server::createDirListHtml(std::string const& dirpath)
{
    size_t pos, len;
    char *path, *pwd;
    std::string temp;
    DIR *dir = opendir(dirpath.c_str());
    if (!dir)
        return "";
    std::string html = "<html><body><h1>Direrctort Listing </h1><ul>";

    struct dirent *entry;
    pwd = getenv("PWD");
    std::cout << "\tPWD = " << pwd << std::endl;
    len = pwd ? strlen(pwd) : 0;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        std::cout << "\tNAME = " << name << std::endl;
        if (name == "." || name == "..")
            continue ;
        path = canonicalize_file_name(name.c_str());
        if (path) {
            std::cout << "\t PATH = " << path << std::endl;
        } else {
            perror("canonicalize_file_name");
            exit(1);
        }
        if (path && pwd) {
            pos = temp.assign(path).find(pwd);
            if (pos != std::string::npos) {
                temp.replace(pos, len, "").insert(0, "dirs/");
                html += "<li><a href=\"" + temp + "\">" + name + "</a></li>\r\n";
            }
        }
        free(path);
        path = NULL;
    }
    html += "</ul>";
    closedir(dir);
    return "<html><body>" + html + "</body></html>";
}
*/

std::string Server::createDirListHtml(std::string const& dirpath) {
    DIR *dirp;
    struct dirent *entry;
    const char *pwd, *dirname;//, *absolute_path;
    std::string html, name, pwdstr, relative_path;

    //std::cout << "CURRENT PWD: " << getcwd(NULL, 100) << ", dirpath: " << dirpath << std::endl;
    dirp = opendir(dirpath.c_str());
    if (!dirp) {
        return perror("opendir"), "";
    }
    pwd = getenv("PWD");
    if (!pwd) {
        return perror("getenv"), closedir(dirp), "";
    }
    pwdstr = pwd;
    if ((dirname = strstr(dirpath.c_str(), pwd))) {
        if (!isDirectory(dirname) || dirpath.size() <= pwdstr.size()) {
            dirname = NULL;
        } else {
            dirname += pwdstr.length();
        }
    }
    std::cout << "\tDIRNAME = " << (dirname ? dirname : "") << ", DIRPATH = " << dirpath << std::endl;
    html = "<html><body><h1>Direrctory Listing </h1><ul>";
    while ((entry = readdir(dirp))) {
        name = entry->d_name;
        if (name == "." || name == "..")
            continue ;
        std::cout << ">>> \tNAME = " << name << std::endl;
//        absolute_path = realpath(name.c_str(), NULL);
//        if (!absolute_path) {
//            if (isDirectory(name)) {
//                absolute_path = name.c_str();
//            } else {
//                handle_error("realpath");
//            }
//        }
//        assert(strstr(absolute_path, pwd));
//        relative_path.assign(absolute_path).replace(0, pwdlen, "");
//        if (dirname) {
//            relative_path.assign(dirname).append(1, '/').append(name);
//        } else {
//            relative_path.assign(name);
//        }
        if (!dirpath.empty() && dirpath != ".") {
            relative_path.assign(dirpath).append(1, '/').append(name);
        } else {
            relative_path.assign(name);
        }
        if (!relative_path.empty() && relative_path[0] == '/') {
            relative_path.erase(0, 1);
        }
        std::cout << "\tRELATIVE_PATH = " << relative_path << std::endl;
        //if (relative_path.find("dirs") == std::string::npos) {
            html += "<li><a href=\"/dirs/" + relative_path + "\">" + name + "</a></li>";
//        } else {
//            html += "<li><a href=\"" + relative_path + "\">" + name + "</a></li>";
//        }
    }
    closedir(dirp);
    html += "</ul></body></html>";
    return html;
}

void Server::serverdirlisting(int clientsocket, const std::string &dirpath)
{
    DIR *dir = opendir(dirpath.c_str());
    if (!dir)
    {
        severerrorpage(clientsocket, 404);
        return ;
    }
    std::string html = "<html><body><h1>Direrctort Listing </h1><ul>";

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue ;
        html += "<li><a href=\"" + name + "\">" + name + "</a></li>";
    }
    html += "</ul></body></html>";
    closedir(dir);
    std::ostringstream oss;
    oss << html.size();
    std::string response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/html\r\n\r\n"
                           "Content-Length: " + oss.str() + "\r\n\r\n"
                           "<html><body>" + html + "</body></html>";
    send(clientsocket, response.c_str(), response.size(), MSG_DONTWAIT);
}

bool Server::isDirectory(std::string const& path)
{
    struct stat statbuf;// struct stat: file status structure

    if (stat(path.c_str(), &statbuf) != 0) // stat: get file status
        return (false);
    return S_ISDIR(statbuf.st_mode); // S_ISDIR: is directory macro in stat.h file
}

bool Server::isdirectory(const std::string &path)
{
    struct stat statbuf;// struct stat: file status structure

    if (stat(path.c_str(), &statbuf) != 0) // stat: get file status
        return (false);
    return S_ISDIR(statbuf.st_mode); // S_ISDIR: is directory macro in stat.h file
}

std::string Server::sanitizepath(const std::string& basedir, const std::string& requestedpath)
{
    char resolvedpath[PATH_MAX];

    std::string fullpath = basedir + "/" + requestedpath;

    if (realpath(fullpath.c_str(), resolvedpath))
    {
        std::string sanitizedpath(resolvedpath);

        // Ensure the resolved path is still within the base directory
        if (sanitizedpath.find(basedir) == 0)
        {
            return (sanitizedpath); // Valid and secure path
        }
    }
    return (""); // Invalid path
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

        std::string sanitizedpath = sanitizepath(rootdir, url);
        if (sanitizedpath.empty())
        {
            severerrorpage(client_socket, 400);
            return ;
        }
        else if (method == "GET")
        {
            std::cout << "GET request received" << std::endl;
            if (isdirectory(sanitizedpath))
            {
                serverdirlisting(client_socket, sanitizedpath);
                return ;
            }
            else
            {
                servestaticfile(client_socket, sanitizedpath);
            }
        }
        else if (method == "POST")
        {
            std::cout << "POST request received" << std::endl;
            size_t start = request.find("\r\n\r\n") + 4; // start of the body
            std::string body = request.substr(start);

            std::ofstream outfile("upload.txt");
            {
                if (outfile)
                {
                    outfile << body;
                    outfile.close();
                }
            }
            std::string response = "HTTP/1.1 200 OK\r\n"
                                "Content-Type: text/html\r\n\r\n"
                                "<html><body><h1>File uploaded successfully</h1></body></html>";
            send(client_socket, response.c_str(), response.size(), MSG_DONTWAIT);

        }
        else if (method == "DELETE")
        {
            std::cout << "DELETE request received" << std::endl;
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
        else
        {
            severerrorpage(client_socket, 405);
        }
    }
    else
    {
        perror("recv failed");
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
            statusmessage = "404 Not Found";
            break ;
        case 500:
            statusmessage = "500 Internal Server Error";
            break ;
        default:
            statusmessage = "400 Bad Request";
            break ;
    }
    std::string errorpagepath = errorpage[statuscode]; // errorpage is a map defined in Server.hpp file error_pages[404] = "/errors/404.html"; error_pages[500] = "/errors/500.html";
    std::ifstream file(errorpagepath.c_str());
    std::string response;

    if (file)
    {
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        std::ostringstream oss;
        oss << content.size();

        response = "HTTP/1.1 " + statusmessage + " " + statusmessage + "\r\n"
                    "Content-Type: text/html\r\n\r\n"
                    "Content-Length: " + oss.str() + "\r\n\r\n"
                    "<html><body>" + content + "</body></html>";
    }
    else
    {
        response = "HTTP/1.1 " + statusmessage + "\r\n"
                    "Content-Type: text/html\r\n\r\n"
                    "<html><body><h1>" + statusmessage + "</h1></body></html>";
    }
    send(clientsocket, response.c_str(), response.size(), MSG_DONTWAIT);
}
