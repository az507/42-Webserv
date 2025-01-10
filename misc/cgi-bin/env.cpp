#include <iostream>
#include <sstream>
#include <cstring>

int main(int argc, char *argv[], char *envp[]) {
    size_t len;
    std::ostringstream oss;

    oss << argc;
    len = strlen("argc: ") + oss.str().length();
    for (int i = 0; argv[i] != NULL; ++i) {
        len += strlen(argv[i]);
    }
    for (int i = 0; envp[i] != NULL; ++i) {
        len += strlen(envp[i]);
    }
    std::cout << "HTTP/1.1 200 OK\r\n";
    std::cout << "Content-Type: text/plain\r\n";
    std::cout << "Connection: keep-alive\r\n";
    std::cout << "Content-Length: " << len << "\r\n\r\n";
    std::cout << "argc: " << argc << "\r\n";
    for (int i = 0; argv[i] != NULL; ++i) {
        std::cout << argv[i] << "\r\n";
    }
    for (int i = 0; envp[i] != NULL; ++i) {
        std::cout << envp[i] << "\r\n";
    }
}
