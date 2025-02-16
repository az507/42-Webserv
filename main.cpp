#include "Server.hpp"
#include "ConfigFile.hpp"
#include "Client.hpp"
#include <list>
#include <limits>
#include <sys/wait.h>

void sigchldHandler(int sig) {
    if (sig == SIGCHLD) {
        while (waitpid(-1, NULL, WNOHANG) > 0);
        if (errno == ECHILD) {
            errno = 0;
        }
    }
}

// static void handleCgiScript(const std::string &scriptPath, int clientFd) {
//     int cgiOutput[2];
//     pid_t pid;

//     // Create a pipe for CGI output
//     if (pipe(cgiOutput) == -1) {
//         perror("pipe");
//         return;
//     }

//     // Set pipes to non-blocking
//     fcntl(cgiOutput[0], F_SETFL, O_NONBLOCK);
//     fcntl(cgiOutput[1], F_SETFL, O_NONBLOCK);

//     if ((pid = fork()) == -1) {
//         perror("fork");
//         close(cgiOutput[0]);
//         close(cgiOutput[1]);
//         return;
//     }

//     if (pid == 0) { // Child process
//         dup2(cgiOutput[1], STDOUT_FILENO); // Redirect stdout to pipe
//         close(cgiOutput[0]);              // Close unused read end
//         close(cgiOutput[1]);

//         // Replace current process with CGI script
//         char *args[] = {const_cast<char *>(scriptPath.c_str()), NULL};
//         char *env[] = {NULL};
//         execve(scriptPath.c_str(), args, env);

//         perror("execve"); // If execve fails
//         exit(1);
//     } else { // Parent process
//         close(cgiOutput[1]); // Close unused write end

//         // Read CGI output in non-blocking mode
//         char buffer[1024];
//         ssize_t bytesRead;
//         while ((bytesRead = read(cgiOutput[0], buffer, sizeof(buffer) - 1)) > 0) {
//             buffer[bytesRead] = '\0';
//             send(clientFd, buffer, bytesRead, 0); // Send CGI output to client
//         }

//         close(cgiOutput[0]); // Close read end

//         // Wait for child process to terminate
//         int status;
//         waitpid(pid, &status, WNOHANG);
//         if (WIFEXITED(status)) {
//             std::cout << "CGI exited with status " << WEXITSTATUS(status) << std::endl;
//         }
//     }
// }

int main(int argc, char *argv[], char *envp[]) {
    const char *node, *service;
    int err, connfd, optval = 1;
    struct addrinfo hints, *res;
    std::vector<ServerInfo> _servers;

    //std::cout.setstate(std::ios_base::failbit);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, &sigchldHandler);
    Client::setEnvp(const_cast<const char **>(envp));

    if (argc == 1) {
        argv[1] = const_cast<char *>("misc/conf/default.conf");
    }
    try {
        _servers = ConfigFile(argv[1]).getServerInfo();
    } catch (std::exception const& e) {
        return std::cerr << e.what() << '\n', 1;
    }
    int idx = 0;
    for (std::vector<ServerInfo>::const_iterator it = _servers.begin(); it != _servers.end(); ++it) {
        std::cout << "\tnumber " << ++idx << '\n';
        assert(!it->routes.empty());
    }
    for (std::vector<ServerInfo>::iterator it = _servers.begin(); it != _servers.end(); ++it) {
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
                it->connfds.push_back(connfd);
                break ;
            }
            if (connfd == -1) {
                std::cerr << "Ip-addr: " << tmp->first << ':' << tmp->second << " cannot be used\n";
            }
            freeaddrinfo(res);
            res = NULL;
        }
    }

    for (std::vector<ServerInfo>::const_iterator it = _servers.begin(); it != _servers.end(); ++it) {
        if (std::find_if(it->connfds.begin(), it->connfds.end(), std::bind2nd(std::ptr_fun(&listen), MAXEVENTS)) != it->connfds.end()) {
            return perror("listen"), 1;
        }
    }

    socklen_t addrlen;
    struct sockaddr addr;
    int nfds, sockfd, max_connfd;
    std::list<Client> clients, temp;
    std::list<Client>::iterator c_it;
    std::vector<struct epoll_event> events(MAXEVENTS);

    Client::setEpollfd(epoll_create(1));
    if (Client::getEpollfd() == -1) {
        return perror("epoll_create"), 1;
    }
    for (std::vector<ServerInfo>::const_iterator it = _servers.begin(); it != _servers.end(); ++it) {
        Client::addOpenFds(it->connfds);
        for (std::vector<int>::const_iterator tmp = it->connfds.begin(); tmp != it->connfds.end(); ++tmp) {
            Client::registerEvent(*tmp, EPOLLIN);
        }
    }
    max_connfd = std::numeric_limits<int>::min();
    for (std::vector<ServerInfo>::const_iterator it = _servers.begin(); it != _servers.end(); ++it) {
        if (!it->connfds.empty()) {
            max_connfd = std::max(max_connfd, *std::max_element(it->connfds.begin(), it->connfds.end()));
        }
    }
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
            if (events[i].data.fd > max_connfd) { // data.fd is a sockfd to send/recv to, not a connfd to accept new connections from
                c_it = std::find_if(clients.begin(), clients.end(), std::bind2nd(std::mem_fun_ref(&Client::operator==), events[i].data.fd));
                if (c_it == clients.end()) { // if reached here, probably is stored in one of clients' passive_fd
                    continue ;
                }
                if (events[i].events & EPOLLIN) {
                    c_it->socketRecv();
                }
                if (*c_it == events[i].data.fd && events[i].events & EPOLLOUT) { // in the middle of socketRecv(), the _activefd can change
                    c_it->socketSend();
                }
                // vvvvv DONT UNCOMMENT THIS, IT WILL BREAK CGI COMPLETELY vvvvvv
                // now, a single Client object can respond to multiple fds because of list<CGI> inside it
                // c_it SHOULD NOT get plucked out of search area of clients list prematurely
//                if (!c_it->isConnClosed()) {
//                    temp.splice(temp.begin(), clients, c_it);
//                }
            } else {
                addrlen = 0;
                memset(&addr, 0, sizeof(addr));
                // possible race condition, where client connection attempt may have dropped between epoll_wait() and accept()
                // if events[i].data.fd is blocking, accept call can potentially block
                // may need to use fcntl() to set fd to non-blocking
                sockfd = accept(events[i].data.fd, &addr, &addrlen);
                if (sockfd == -1) {
                    handle_error("accept");
                }
                Client::addOpenFd(sockfd);
                Client::registerEvent(sockfd, EPOLLIN | EPOLLOUT);
                temp.push_back(Client(events[i].data.fd, sockfd, _servers));
            }
        }
        // can also put timeout condition in isConnClosed() func so can remove all dead or inactive connections in 1 call
        c_it = std::remove_if(clients.begin(), clients.end(), std::mem_fun_ref(&Client::isConnClosed));
        if (c_it != clients.end()) {
            std::for_each(c_it, clients.end(), std::mem_fun_ref(&Client::closeFds));
            clients.erase(c_it, clients.end());
        }
        std::fill(events.begin(), events.end(), (struct epoll_event){});
        temp.splice(clients.end(), temp, temp.begin(), temp.end());
        while (waitpid(-1, NULL, WNOHANG) > 0) ;
    }

    for (std::vector<ServerInfo>::const_iterator it = _servers.begin(); it != _servers.end(); ++it) {
        for (std::vector<int>::const_iterator tmp = it->connfds.begin(); tmp != it->connfds.end(); ++tmp) {
            if (epoll_ctl(Client::getEpollfd(), EPOLL_CTL_DEL, *tmp, NULL) == -1) {
                return perror("3) epoll_ctl"), 1;
            }
        }
    }
    Client::closeOpenFds();
}
