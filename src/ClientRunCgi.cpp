#include "Client.hpp"

void Client::runCgiScript(std::pair<std::string, std::string> const& reqInfo) {
    int pipefds[2];
    int stdin_fd, stdout_fd;

    stdin_fd = dup(STDIN_FILENO);
    if (stdin_fd == -1) {
        handle_error("1) dup");
    }
    stdout_fd = dup(STDOUT_FILENO);
    if (stdout_fd == -1) {
        handle_error("2) dup");
    }
    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, pipefds) == -1) {
        handle_error("socketpair");
    }
    switch (fork()) {
        case -1:        handle_error("fork");
        case 0:         close(pipefds[1]); close(stdin_fd); close(stdout_fd);
                        executeCgi(pipefds[0], reqInfo);
                        throw std::exception(); // force stack unwinding of child process if execve failed
        default:        close(pipefds[0]);
                        registerEvent(pipefds[1], EPOLLIN | EPOLLOUT);
    }
//    std::swap(active_fd, passive_fd = pipefds[1]);
//    send_it = msg_body.begin();
//    send_ite = msg_body.end();
//    setIOState(SEND_CGI);
//    setPState(HEADERS);
    std::swap(active_fd, passive_fd = pipefds[1]);
    if (http_method == POST_METHOD) {
        send_it = msg_body.begin();
        send_ite = msg_body.end();
        setIOState(SEND_CGI);
    } else {
        setIOState(RECV_CGI);
    }
    setPState(HEADERS);
    if (dup2(STDIN_FILENO, stdin_fd) == -1) {
        handle_error("1) dup2");
    }
    if (dup2(STDOUT_FILENO, stdout_fd) == -1) {
        handle_error("2) dup2");
    }
    if (close(stdin_fd) == -1) {
        handle_error("1) close");
    }
    if (close(stdout_fd) == -1) {
        handle_error("2) close");
    }
}

#include <fcntl.h>
// reqInfo.first = query_str, .second = path_info
void Client::executeCgi(int pipefd, std::pair<std::string, std::string> const& reqInfo) {
    std::vector<char *> argv(3, NULL);
    std::vector<char *> envp;

//    int fd = open("outfile", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
//    if (fd == -1) {
//        handle_error("open");
//    }
//    if (dup2(fd, STDOUT_FILENO) == -1) {
    std::cout << "pipefd: " << pipefd << std::endl;
    if (dup2(pipefd, STDOUT_FILENO) == -1) {
        handle_error("dup2");
    }
//    if (close(fd) == -1) {
//        handle_error("fd close");
//    }
//    if (dup2(pipefd, STDIN_FILENO) == -1 || dup2(pipefd, STDOUT_FILENO) == -1) {
//        handle_error("dup2");
//    }
    //std::cout.sync_with_stdio(false);
    //std::cin.tie(NULL);
//    if (dup2(pipefd, STDIN_FILENO) == -1 || dup2(pipefd, STDOUT_FILENO) == -1) {
//        handle_error("dup2");
//    }
    if (close(pipefd) == -1) {
        handle_error("close");
    }
    std::cout << "isatty(stdout): " << isatty(STDOUT_FILENO) << '\n';
    std::cout << "isatty: " << strerror(errno) << '\n';
    errno = 0;
    std::cout << "isatty(stdin): " << isatty(STDIN_FILENO) << '\n';
    std::cout << "isatty: " << strerror(errno) << '\n';
    errno = 0;
//    printf("this should not be printed to the terminal\n");
//    std::cout << "isatty(stdout): " << isatty(STDOUT_FILENO) << '\n';
//    perror("isatty");
//    std::cout << "isatty(stdin): " << isatty(STDIN_FILENO) << '\n';
//    perror("isatty");
    argv[0] = strdup(request_uri.c_str());
    argv[1] = strdup(reqInfo.second.c_str());
    // need to provide more env vars to cgi script based on rfc spec
    for (int i = 0; Client::envp[i] != NULL; ++i) {
        envp.push_back(strdup(Client::envp[i]));
    }
    envp.push_back(strdup(std::string("QUERY_STRING=").append(reqInfo.first).c_str()));
    envp.push_back(NULL);
    fflush(stdout);
    if (execve(argv[0], argv.data(), envp.data()) == -1) {
        perror(argv[0]);
    }
    std::cout << "not supposed to be here" << std::endl;
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
