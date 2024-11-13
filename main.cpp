#include <netdb.h>
#include <sys/types.h>
#include "Server.hpp"
#include "ConfigFile.hpp"

int main(int argc, char *argv[]) {
    std::vector<int> sockfds;
    struct addrinfo hints, *res;
    std::vector<ServerInfo> servers;
    int err, sockfd, optval = 1, epollfd;

    if (argc == 1) {
        argv[1] = const_cast<char *>("default.conf");
    }
    try {
        servers = ConfigFile(argv[1]).getServerInfo();
    } catch (const std::exception& e) {
        return std::cerr << e.what() << '\n', 1;
    }
    for (std::vector<ServerInfo>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
        for (std::vector<std::pair<std::string, std::string> >::const_iterator tmp = it->ip_addrs.begin(); tmp != it->ip_addrs.end(); ++tmp) {
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags = AI_PASSIVE;
            assert(!(tmp->first.empty() && tmp->second.empty()));
            err = getaddrinfo(tmp->first.empty() ? NULL : tmp->first.c_str(),
                tmp->second.empty() ? NULL : tmp->second.c_str(), &hints, &res);
            if (err) {
                return std::cerr << "getaddrinfo: " << gai_strerror(err) << '\n', 1;
            }
            for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
                sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
                if (sockfd == -1) {
                    perror("socket");
                    continue ;
                }
                if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
                    perror("setsockopt");
                    close(sockfd);
                    break ;
                }
                if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                    perror("bind");
                    close(sockfd);
                    sockfd = -1;
                    continue ;
                }
                sockfds.push_back(sockfd);
                break ;
            }
            if (sockfd == -1) {
                std::cerr << "Ip-addr: " << tmp->first << ':' << tmp->second << " cannot be used\n";
            }
            freeaddrinfo(res);
            res = NULL;
        }
    }
    epollfd = epoll_create(1);
    if (epollfd == -1) {
        return perror("epoll_create"), 1;
    }
    struct epoll_event ev;
    for (std::vector<int>::const_iterator it = sockfds.begin(); it != sockfds.end(); ++it) {
        ev.data.fd = *it;
        ev.events = EPOLLIN;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, *it, &ev) == -1) {
            return perror("epoll_ctl"), 1;
        }
    }
    for (std::vector<int>::const_iterator it = sockfds.begin(); it != sockfds.end(); ++it) {
        if (epoll_ctl(epollfd, EPOLL_CTL_DEL, *it, NULL) == -1) {
            return perror("epoll_ctl"), 1;
        }
    }
    std::for_each(sockfds.begin(), sockfds.end(), &close);
    close(epollfd);
}
