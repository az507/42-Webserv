#include "Client.hpp"

void Client::runCgiScript(std::pair<std::string, std::string> const& reqInfo) {
    int pipefds[2];

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, pipefds) == -1) {
        handle_error("socketpair");
    }
    std::cout << "\tCHANGING DIR TO " << route->root << " IN CHILD PROCESS" << std::endl;
    switch (fork()) {
        case -1:        handle_error("fork");
        case 0:         if (route && chdir(route->root.c_str()) == -1) {
                            handle_error("chdir");
                        }
                        close(pipefds[0]);
                        executeCgi(pipefds[1], reqInfo);
                        throw std::exception(); // force stack unwinding of child process if execve failed
        default:        close(pipefds[1]);
                        registerEvent(pipefds[0], EPOLLIN | EPOLLOUT);
    }
    std::swap(active_fd, passive_fd = pipefds[0]);
    std::cout << "after forking, active_fd: " << active_fd << ", passive_fd: " << passive_fd << std::endl;
    if (http_method == POST_METHOD) {
        send_it = msg_body.begin();
        send_ite = msg_body.end();
        setIOState(SEND_CGI);
    } else {
        msg_body.clear();
        setIOState(RECV_CGI);
    }
}

std::vector<char *> Client::initCgiEnv(std::pair<std::string, std::string> const& reqInfo) const {
    bool flag;
    size_t pos;
    std::string temp;
    std::vector<char *> envp;
    std::map<std::string, std::string>::const_iterator c_ite;

    // need to provide more env vars to cgi script based on rfc spec
    for (int i = 0; Client::envp[i] != NULL; ++i) {
        envp.push_back(strdup(Client::envp[i]));
    }
    envp.push_back(strdup(std::string("PATH_INFO=").append(reqInfo.first).c_str()));
    envp.push_back(strdup(std::string("QUERY_STRING=").append(reqInfo.second).c_str()));
    if (route) {
        envp.push_back(strdup(std::string("UPLOAD_DIR=").append(route->upload_dir).c_str()));
    }
    switch (http_method) {
        case GET_METHOD:        envp.push_back(strdup("REQUEST_METHOD=GET")); break ;
        case POST_METHOD:       envp.push_back(strdup("REQUEST_METHOD=POST")); break ;
        case DELETE_METHOD:     envp.push_back(strdup("REQUEST_METHOD=DELETE")); break ;
        default:                std::terminate();
    }
    flag = false;
    c_ite = headers.end();
    for (std::map<std::string, std::string>::const_iterator c_it = headers.begin(); c_it != c_ite; ++c_it) {
        temp.clear();
        temp.reserve(c_it->first.length() + c_it->second.length() + 1);
        std::transform(c_it->first.begin(), c_it->first.end(), std::back_inserter(temp), &toupper);
        if ((pos = temp.find('-')) != std::string::npos) {
            temp[pos] = '_';
        }
        temp.push_back('=');
        if (!flag && c_it->first == "Content-Length") {
            // putting Content-Length header value doesnt seem to matter for Python CGI module handling File Uploads
            // seems that setting a bunch of env vars for the cgi process fixed issue (maybe "Content-Type" header?)
            if (http_method == POST_METHOD) {
                envp.push_back(strdup((temp += str_convert(msg_body.size())).c_str()));
            } else {
                envp.push_back(strdup((temp += "0").c_str()));
            }
            flag = true;
        } else {
            envp.push_back(strdup((temp += c_it->second).c_str()));
        }
    }
    envp.push_back(NULL);
    return envp;
}

// reqInfo.first = query_str, .second = path_info
void Client::executeCgi(int pipefd, std::pair<std::string, std::string> const& reqInfo) {
    std::vector<char *> envp;
    std::vector<char *> argv(3, NULL);

    if (dup2(pipefd, STDOUT_FILENO) == -1 || dup2(pipefd, STDIN_FILENO) == -1) {
        handle_error("dup2");
    }
    if (close(pipefd) == -1) {
        handle_error("close");
    }
    argv[0] = strdup(request_uri.c_str());
    argv[1] = strdup(reqInfo.second.c_str());
    envp = initCgiEnv(reqInfo);
    std::cerr << "before) from cgi process, argv[1] = " << argv[1] << ", pwd = " << getcwd(NULL, 200) << '\n';
    if (argv[0][0]) {
        char *ptr = strrchr(argv[0], '/');
        if (ptr && !ptr[1]) {
            *ptr = '\0';
        }
    }
    std::cerr << "after) from cgi process, argv[1] = " << argv[1] << ", pwd = " << getcwd(NULL, 200) << '\n';
    if (execve(argv[0], argv.data(), envp.data()) == -1) {
        perror(argv[0]);
        std::copy(argv.begin(), argv.end() - 1, std::ostream_iterator<char *>(std::cout, "\n"));
    }
    std::for_each(argv.begin(), argv.end(), &free);
    std::for_each(envp.begin(), envp.end(), &free);
    // may need to close all other open fds to prevent fd leak
    //exit(EXIT_FAILURE); // may cause memory leak
}

void Client::registerEvent(int fd, uint32_t events) {
    struct epoll_event ev;

    (void)events;
    memset(&ev, 0, sizeof(ev));
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLOUT;
    if (epoll_ctl(Client::epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        handle_error("RE: epoll_ctl");
    }
}
