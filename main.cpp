#include "Server.hpp"
#include "ConfigFile.hpp"
#define BUFSIZE 1024
#define handle_error(err) \
    do { perror(err); exit(EXIT_FAILURE); } while (0);

int main(int argc, char *argv[]) {
    std::vector<int> connfds;
    const char *node, *service;
    int err, connfd, optval = 1;
    struct addrinfo hints, *res;
    std::vector<ServerInfo> servers;

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
    bool msgsent = false;
    char buffer[BUFSIZE];
    struct sockaddr addr;
    std::list<int> clients;
    std::vector<int> sockfds;
    int nfds, sockfd, min_sockfd;
    std::vector<int>::iterator s_it;
    struct epoll_event ev, events[MAXEVENTS];
    std::string msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\nSent from server to browser";

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
        nfds = epoll_wait(Client::getEpollfd(), events, MAXEVENTS, -1);
        if (nfds == -1) {
            return perror("epoll_wait"), 1;
        }
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd >= min_sockfd) {{ // data.fd is a sockfd to send/recv to, not a connfd to accept new connections from
                std::find_if(clients.begin(), clients.end(), std::bind2nd(std::mem_fun_ref(&Client::operator==), events[i],data.fd));
                if (events[i].events & EPOLLIN) {
                }
                if (events[i].events & EPOLLOUT) {
                }
            } else {
                //addrlen = 0;
                memset(&addr, 0, sizeof(addr));
                sockfd = accept(*s_it, &addr, &(addrlen = 0));
                if (sockfd == -1) {
                    return perror("accept"), 1;
                }
                ev.data.fd = sockfd;
                ev.events = EPOLLIN | EPOLLOUT;
                if (epoll_ctl(Client::getEpollfd(), EPOLL_CTL_ADD, sockfd, &ev) == -1) {
                    return perror("2) epoll_ctl"), 1;
                }
                sockfds.push_back(sockfd);
                clients.push_bacK(Client(sockfd, servers));
            }
        }
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
    std::for_each(sockfds.begin(), sockfds.end(), &close);
    close(Client::getEpollfd());
}
