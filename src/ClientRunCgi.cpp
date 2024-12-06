#include "Client.hpp"

void Client::runCgiScript(std::pair<std::string, std::string> const& reqInfo) {
    int pipefds[2];

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, pipefds) == -1) {
        handle_error("socketpair");
    }
    switch (fork()) {
        case -1:        handle_error("fork");
        case 0:         close(pipefds[1]);
                        executeCgi(pipefds[0], reqInfo);
                        throw std::exception(); // force stack unwinding of child process
        default:        close(pipefds[0]);
                        registerEvent(pipefds[1], EPOLLIN | EPOLLOUT);
    }
    std::swap(active_fd, passive_fd = pipefds[1]);
    send_it = msg_body.begin();
    send_ite = msg_body.end();
    setIOState(SEND_CGI);
    setPState(HEADERS);
}

// reqInfo.first = query_str, .second = path_info
void Client::executeCgi(int pipefd, std::pair<std::string, std::string> const& reqInfo) {
    std::vector<char *> argv(3, NULL);
    std::vector<char *> envp;

    if (dup2(pipefd, STDIN_FILENO) == -1 || dup2(pipefd, STDOUT_FILENO) == -1) {
        handle_error("dup2");
    }
    if (close(pipefd) == -1) {
        handle_error("close");
    }
    argv[0] = strdup(request_uri.c_str());
    argv[1] = strdup(reqInfo.second.c_str());
    // need to provide more env vars to cgi script based on rfc spec
    for (int i = 0; Client::envp[i] != NULL; ++i) {
        envp.push_back(strdup(envp[i]));
    }
    envp.push_back(strdup(std::string("QUERY_STRING=").append(reqInfo.first).c_str()));
    envp.push_back(NULL);
    if (execve(argv[0], argv.data(), envp.data()) == -1) {
        perror(argv[0]);
    }
    std::for_each(argv.begin(), argv.end(), &free);
    std::for_each(envp.begin(), envp.end(), &free);
    // may need to close all other open fds to prevent fd leak
    //exit(EXIT_FAILURE); // may cause memory leak
}

void Client::registerEvent(int fd, uint32_t events) {
    struct epoll_event ev;

    memset(&ev, 0, sizeof(ev));
    ev.data.fd = fd;
    ev.events = events;
    if (epoll_ctl(Client::epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        handle_error("RE: epoll_ctl");
    }
}
