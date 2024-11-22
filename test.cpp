#include <netdb.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <unistd.h>
#include "include/ConfigFile.hpp"

int main(int, char **, char *envp[]) {
    pid_t pid;
    struct addrinfo hints, *res;
    socklen_t addrlen;
    struct sockaddr addr;
    int err, connfd, sockfd, optval = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    err = getaddrinfo(NULL, "10100", &hints, &res);
    if (err) {
        return std::cerr << "getaddrinfo: " << gai_strerror(err) << '\n', 1;
    }
    connfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (connfd == -1) {
        return perror("socket"), 1;
    }
    if (setsockopt(connfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        return perror("setsockopt"), 1;
    }
    if (bind(connfd, res->ai_addr, res->ai_addrlen) == -1) {
        return perror("bind"), 1;
    }
    freeaddrinfo(res);
    if (listen(connfd, 10) == -1) {
        return perror("listen"), 1;
    }
    sockfd = accept(connfd, &addr, &addrlen);
    if (sockfd == -1) {
        return perror("accept"), 1;
    }
    std::cout << "ip-addr: " << ntohl(((struct sockaddr_in *)&addr)->sin_addr.s_addr) << std::endl;
    std::cout << "port: " << ntohs(((struct sockaddr_in *)&addr)->sin_port) << std::endl;
    std::cout << "sockfd: " << sockfd << std::endl;
    pid = fork();
    if (pid == -1) {
        return perror("fork"), 1;
    }
    if (!pid) {
        dup2(sockfd, STDOUT_FILENO);
        close(sockfd);
        close(connfd);
//        char * const arr[3] = {
//            const_cast<char *const>("/bin/ls"),
//            const_cast<char *const>("-al"),
//            NULL
//        };
        char const * arr[3] = {
            "/bin/ls",
            "-al",
            NULL
        };
        //execve(arr[0], (char *const [])arr, envp);
        std::cout << "200 HTTP/1.1 OK\r\nContent-type: text/html\r\n\r\n";
        execve(arr[0], (char *const *)arr, envp);
    } else {
        int wstatus;
        waitpid(pid, &wstatus, 0);
        if (WIFEXITED(wstatus)) {
            std::cout << "status: " << WEXITSTATUS(wstatus) << std::endl;
        } else if (WIFSIGNALED(wstatus)) {
            std::cout << "sig status: " << WTERMSIG(wstatus) << std::endl;
        }
        close(sockfd);
        close(connfd);
    }
}
