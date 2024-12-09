#include "Server.hpp"
#include "ConfigFile.hpp"
#include "Client.hpp"
#include <list>
#include <limits>
#include <sys/wait.h>

void sigchldHandler(int sig) {
    pid_t pid;

    if (sig == SIGCHLD) {
        while ((pid = waitpid(-1, NULL, WNOHANG)) > 0);
        if (pid == -1) {
            errno = 0;
        }
    }
}

// for some reason, there are problems with displaying .svg files, may need extra info in the html doc
int main(int argc, char *argv[], char *envp[]) {
    //std::vector<int> connfds;
    const char *node, *service;
    int err, connfd, optval = 1;
    struct addrinfo hints, *res;
    std::vector<ServerInfo> servers;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, &sigchldHandler);
    Client::setEnvp(const_cast<const char **>(envp));

    if (argc == 1) {
        argv[1] = const_cast<char *>("misc/default.conf");
    }
    try {
        servers = ConfigFile(argv[1]).getServerInfo();
    } catch (std::exception const& e) {
        return std::cerr << e.what() << '\n', 1;
    }
    int idx = 0;
    for (std::vector<ServerInfo>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
        std::cout << "\tnumber " << ++idx << '\n';
        assert(!it->routes.empty());
    }
    for (std::vector<ServerInfo>::iterator it = servers.begin(); it != servers.end(); ++it) {
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
                connfd = socket(p->ai_family, p->ai_socktype | SOCK_NONBLOCK | SOCK_CLOEXEC, p->ai_protocol);
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
                it->connfds.push_back(connfd);
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

    for (std::vector<ServerInfo>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
        if (std::find_if(it->connfds.begin(), it->connfds.end(), std::bind2nd(std::ptr_fun(&listen), MAXEVENTS)) != it->connfds.end()) {
            return perror("listen"), 1;
        }
    }
    const char *pwd = NULL;
    for (int i = 0; envp[i] != NULL; ++i) {
        if (!strncmp(envp[i], "PWD=", 4)) {
            pwd = strchr(envp[i], '=') + 1;
            break ;
        }
    }
    if (pwd) {
        std::cout << "PWD: " << pwd << '\n';
    }
    for (std::vector<ServerInfo>::iterator it = servers.begin(); it != servers.end(); ++it) {
        for (std::vector<RouteInfo>::iterator tmp = it->routes.begin(); tmp != it->routes.end(); ++tmp) {
            if (!tmp->root.empty() && tmp->root.find('/') == std::string::npos && pwd) {
                tmp->root.insert(tmp->root.begin(), '/');
                tmp->root.insert(0, pwd);
                //std::cout << "tmp->root: " << tmp->root << '\n';
            }
        }
    }

    socklen_t addrlen;
    struct sockaddr addr;
    int nfds, sockfd, max_connfd;
    std::list<Client> clients, temp;
    std::list<Client>::iterator c_it;
    struct epoll_event ev;//, events[MAXEVENTS];
    std::vector<struct epoll_event> events(MAXEVENTS);

    // timeout recv/write after 10s
//    struct timeval tv;
//    tv.tv_sec = 10;
//    tv.tv_usec = 0;

    Client::setEpollfd(epoll_create(1));
    if (Client::getEpollfd() == -1) {
        return perror("epoll_create"), 1;
    }
    for (std::vector<ServerInfo>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
        for (std::vector<int>::const_iterator tmp = it->connfds.begin(); tmp != it->connfds.end(); ++tmp) {
            memset(&ev, 0, sizeof(ev));
            ev.data.fd = *tmp;
            ev.events = EPOLLIN;
            if (epoll_ctl(Client::getEpollfd(), EPOLL_CTL_ADD, *tmp, &ev) == -1) {
                return perror("1) epoll_ctl"), 1;
            }
        }
    }
    max_connfd = std::numeric_limits<int>::min();
    for (std::vector<ServerInfo>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
        if (!it->connfds.empty()) {
            max_connfd = std::max(max_connfd, *std::max_element(it->connfds.begin(), it->connfds.end()));
        }
    }
    //std::copy(servers.begin(), servers.end(), std::ostream_iterator<ServerInfo>(std::cout, "\n"));
    while (1) {
        nfds = epoll_wait(Client::getEpollfd(), events.data(), events.size(), -1); // if not -1, timeout set to 10s here
        if (nfds == -1) {
            if (errno == EINTR) { // epoll_wait interrupted by a signal (probably SIGCHLD, as SIGCHLD handler is installed)
                errno = 0;
                continue ;
            }
            return perror("epoll_wait"), 1;
        }
        for (int i = 0; i < nfds; ++i) {
//            std::cout << "events[" << i << "].data.fd = " << events[i].data.fd << ", max_connfd: " << max_connfd << '\n';
            if (events[i].data.fd > max_connfd) { // data.fd is a sockfd to send/recv to, not a connfd to accept new connections from
                c_it = std::find_if(clients.begin(), clients.end(), std::bind2nd(std::mem_fun_ref(&Client::operator==), events[i].data.fd));
                //assert(c_it != clients.end());
                if (c_it == clients.end()) { // if reached here, probably is stored in one of clients' passive_fd
//                    std::cout << "event fd: " << events[i].data.fd << '\n';
//                    std::copy(clients.begin(), clients.end(), std::ostream_iterator<Client>(std::cout, "\n"));
//                    exit(1);
                    continue ;
                }
                if (events[i].events & EPOLLIN) {
                    c_it->socketRecv();
                }
                if (*c_it != events[i].data.fd) { // in the middle of socketRecv(), the active_fd can change
                    continue ;
                }
                if (events[i].events & EPOLLOUT) {
                    c_it->socketSend();
                }
                temp.splice(temp.begin(), clients, c_it);
            } else {
                addrlen = 0;
                memset(&addr, 0, sizeof(addr));
                // possible race condition, where client connection attempt may have dropped between epoll_wait() and accept()
                // if events[i].data.fd is blocking, accept call can potentially block
                // may need to use fcntl() to set fd to non-blocking
                sockfd = accept(events[i].data.fd, &addr, &addrlen);
                //std::cout << "accept() called\n";
                if (sockfd == -1) {
                    handle_error("accept");
                }
                std::cout << "event fd used in accept(): " << events[i].data.fd << '\n';
                std::cout << "port nbr: " << ntohs(((struct sockaddr_in *)&addr)->sin_port) << '\n';
                std::cout << "ip addr: " << ntohl( ((struct sockaddr_in *)&addr)->sin_addr.s_addr) << '\n';
                // not sure if this two setsockopt()s are needed when MSG_DONTWAIT is set for recv/send
//                if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
//                    handle_error("setsockopt recv");
//                }
//                if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
//                    handle_error("setsockopt send");
//                }
//                if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER
                ev.data.fd = sockfd;
                ev.events = EPOLLIN | EPOLLOUT;
                if (epoll_ctl(Client::getEpollfd(), EPOLL_CTL_ADD, sockfd, &ev) == -1) {
                    return perror("2) epoll_ctl"), 1;
                }
                clients.push_back(Client(events[i].data.fd, servers));
                clients.back().setActiveFd(sockfd);
            }
        }
        // can also put timeout condition in isConnClosed() func so can remove all dead or inactive connections in 1 call
        c_it = std::remove_if(clients.begin(), clients.end(), std::mem_fun_ref(&Client::isConnClosed));
        if (c_it != clients.end()) {
            std::cout << "size of clients list: " << clients.size() << ", number of dead clients: " << std::distance(c_it, clients.end()) << std::endl;
            std::for_each(c_it, clients.end(), std::mem_fun_ref(&Client::closeFds));
            clients.erase(c_it, clients.end());
            std::cout << "size of clients after: " << clients.size() << std::endl;
        }
        clients.splice(clients.end(), temp, temp.begin(), temp.end());
        std::fill(events.begin(), events.end(), (struct epoll_event){});
        // c_it = std::remove_if(clients.begin(), clients.end(), std::mem_fun_ref(&Client::isTimedout));
        // clients.erase(c_it, clients.end());
    }

    for (std::vector<ServerInfo>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
        for (std::vector<int>::const_iterator tmp = it->connfds.begin(); tmp != it->connfds.end(); ++tmp) {
            if (epoll_ctl(Client::getEpollfd(), EPOLL_CTL_DEL, *tmp, NULL) == -1) {
                return perror("3) epoll_ctl"), 1;
            }
        }
    }
    for (std::vector<ServerInfo>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
        std::for_each(it->connfds.begin(), it->connfds.end(), &close);
    }
    std::for_each(clients.begin(), clients.end(), std::mem_fun_ref(&Client::closeFds));
    close(Client::getEpollfd());
}
