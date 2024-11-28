#include "Server.hpp"
#include "ConfigFile.hpp"
#define BUFSIZE 1024
#define handle_error(err) \
    do { perror(err); exit(EXIT_FAILURE); } while (0);

void sigchldHandler(int sig) {

    if (sig == SIGCHLD) {
        while (waitpid(-1, NULL, WNOHANG) > 0) {
            ;
        }
    }
}

int main(int argc, char *argv[]) {
    std::vector<int> connfds;
    const char *node, *service;
    int err, connfd, optval = 1;
    struct addrinfo hints, *res;
    std::vector<ServerInfo> servers;

    Client::setEnvp(envp);
    signal(SIGCHLD, &sigchldHandler);

    if (argc == 1) {
        argv[1] = const_cast<char *>("default.conf");
    }
    try {
        servers = ConfigFile(argv[1]).getServerInfo();
    } catch (std::exception const& e) {
        return std::cerr << e.what() << '\n', 1;
    }
    for (std::vector<ServerInfo>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
        assert(!it->routes.empty());
    }
    for (std::vector<ServerInfo>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
        for (std::vector<std::pair<std::string, std::string> >::const_iterator tmp = it->ip_addrs.begin(); tmp != it->ip_addrs.end(); ++tmp) {
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags = AI_PASSIVE;

            node = tmp->first.empty() ? NULL : tmp->first.c_str();
            service = tmp->second.empty() ? NULL : tmp->second.c_str();
            assert(node || service);

            err = getaddrinfo(node, service, &hints, &res);
            if (err) {
                return std::cerr << "getaddrinfo: " << gai_strerror(err) << '\n', 1;
            }
            for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
                connfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
                if (connfd == -1) {
                    perror("socket");
                    continue ;
                }
                if (setsockopt(connfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
                    perror("setsockopt");
                    close(connfd);
                    break ;
                }
                if (bind(connfd, p->ai_addr, p->ai_addrlen) == -1) {
                    perror("bind");
                    close(connfd);
                    connfd = -1;
                    continue ;
                }
                connfds.push_back(connfd);
//                if (node)
//                    std::cout << "node: " << node << '\n';
//                if (service)
//                    std::cout << "service: " << service << '\n';
//                std::cout << '\n';
                break ;
            }
            if (connfd == -1) {
                std::cerr << "Ip-addr: " << tmp->first << ':' << tmp->second << " cannot be used\n";
            }
            freeaddrinfo(res);
            res = NULL;
        }
    }

    if (std::find_if(connfds.begin(), connfds.end(), std::bind2nd(std::ptr_fun(&listen), MAXEVENTS)) != connfds.end()) {
        return perror("listen"), 1;
    }

    socklen_t addrlen;
    struct sockaddr addr;
    //std::vector<int> sockfds;
    int nfds, sockfd, min_sockfd;
    std::list<Client> clients;
    std::list<Client>::iterator c_it;
    struct epoll_event ev;//, events[MAXEVENTS];
    std::vector<struct epoll_event> events(MAXEVENTS);

    // timeout recv/write after 10s
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;

    Client::setEpollfd(epoll_create(1));
    if (Client::getEpollfd() == -1) {
        return perror("epoll_create"), 1;
    }
    for (std::vector<int>::const_iterator it = connfds.begin(); it != connfds.end(); ++it) {
        ev.data.fd = *it;
        ev.events = EPOLLIN;
        if (epoll_ctl(Client::getEpollfd(), EPOLL_CTL_ADD, *it, &ev) == -1) {
            return perror("1) epoll_ctl"), 1;
        }
    }
    min_sockfd = std::max_element(connfds.begin(), connfds.end());
    while (1) {
        nfds = epoll_wait(Client::getEpollfd(), events.data(), events.size(), 1e6); // timeout set to 10s here
        if (nfds == -1) {
            return perror("epoll_wait"), 1;
        }
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd > min_sockfd) { // data.fd is a sockfd to send/recv to, not a connfd to accept new connections from
                c_it = std::find_if(clients.begin(), clients.end(), std::bind2nd(std::mem_fun_ref(&Client::operator==), events[i].data.fd));
                assert(c_it != clients.end());
                if (events[i].events & EPOLLIN) {
                    c_it->socketRecv();
                }
                if (events[i].events & EPOLLOUT) {
                    c_it->socketSend();
                }
            } else {
                //addrlen = 0;
                memset(&addr, 0, sizeof(addr));
                // possible race condition, where client connection attempt may have dropped between epoll_wait() and accept()
                // if events[i].data.fd is blocking, accept call can potentially block
                // may need to use fcntl() to set fd to non-blocking
                sockfd = accept(events[i].data.fd, &addr, &(addrlen = 0));
                if (sockfd == -1) {
                    handle_error("accept");
                }
                // not sure if this two setsockopt()s are needed when MSG_DONTWAIT is set for recv/send
                if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
                    handle_error("setsockopt recv");
                }
                if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
                    handle_error("setsockopt send");
                }
                ev.data.fd = sockfd;
                ev.events = EPOLLIN | EPOLLOUT;
                if (epoll_ctl(Client::getEpollfd(), EPOLL_CTL_ADD, sockfd, &ev) == -1) {
                    return perror("2) epoll_ctl"), 1;
                }
                //sockfds.push_back(sockfd);
                clients.push_bacK(Client(sockfd, servers));
            }
        }
        std::for_each(clients.begin(), clients.end(), std::mem_fun_ref(&Client::parse));
        std::for_each(clients.begin(), clients.end(), std::mem_fun_ref(&Client::executeCgi));

        pid_t pid = fork();
        switch (pid) {
            case -1:        handle_error("fork");
            case 0:         execute_script(), return 1; // probably can setup a sighandler that calls waitpid() when SIGCHLD received
            default:        // registerEvent(), carry on
        
        memset(&ev, 0, sizeof(ev));
        ev.data.fd = pipefd;
        ev.events = EPOLLIN;
        if (epoll_ctl(Client::getEpollfd(), EPOLL_CTL_ADD, pipefd, &ev) == -1) {
            handle_error("epoll_ctl");
        }

        // c_it = std::remove_if(clients.begin(), clients.end(), std::mem_fun_ref(&Client::isTimedout));
        // clients.erase(c_it, clients.end());
    }

    for (std::vector<int>::const_iterator it = connfds.begin(); it != connfds.end(); ++it) {
        if (epoll_ctl(Client::getEpollfd(), EPOLL_CTL_DEL, *it, NULL) == -1) {
            return perror("3) epoll_ctl"), 1;
        }
    }
    for (std::vector<int>::const_iterator it = sockfds.begin(); it != sockfds.end(); ++it) {
        if (epoll_ctl(Client::getEpollfd(), EPOLL_CTL_DEL, *it, NULL) == -1) {
            return perror("4) epoll_ctl"), 1;
        }
    }
    std::for_each(connfds.begin(), connfds.end(), &close);
//    std::for_each(sockfds.begin(), sockfds.end(), &close);
    close(Client::getEpollfd());
}
