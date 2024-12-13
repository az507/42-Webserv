#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#define handle_error(err) \
    do { perror(err); exit(EXIT_FAILURE); } while (0);

std::string getUploadFilepath(const char *dirname) {
    static uint64_t file_idx = 0;

    return (std::ostringstream(dirname) << "/entry " << file_idx++).str();
}

std::streamsize getFileLength(const char *filename) {
    return std::ifstream(filename, std::ios::ate | std::ios::binary).tellg();
}

void handleFileUpload() {
    struct stat statbuf;
    std::ofstream outfile;
    const char *upload_dir;
    static const char *dirname = "uploads";

    upload_dir = std::getenv("UPLOAD_STORE");
    if (!upload_dir && stat(dirname, &statbuf) == -1) {
        if (mkdir(dirname, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1) {
            handle_error("mkdir");
        }
        upload_dir = dirname;
    }
    outfile.open(getUploadFilepath(upload_dir.c_str()));
    if (!outfile.is_open()) {
        handle_error("open outfile");
    }
    std::copy(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>(), std::ostreambuf_iterator<char>(outfile));
    std::cout << "HTTP/1.1 200 OK\r\n";
    std::cout << "Content-Length: 0\r\n";
    std::cout << "Content-Type: text/plain\r\n";
    std::cout << "Connection: keep-alive\r\n\r\n";
}

void getRequestedFile(const char *filename) {
    std::ifstream infile(filename);

    if (!infile.is_open()) {
        handle_error("open infile");
    }
    std::cout << "HTTP/1.1 200 OK\r\n";
    std::cout << "Content-Length: " << getFileLength(filename) << "\r\n";
    std::cout << "Content-Type: " << getFileExtension(filename) << "\r\n";
    std::cout << "Connection: keep-alive\r\n\r\n";
    std::copy(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>(), std::ostream_iterator<char>(std::cout));
}

int main(int argc, char *argv[]) {
    char *http_method;

    if (argc < 2) {
        return 1;
    }
    http_method = std::getenv("HTTP_METHOD");
    assert(http_method != NULL);
    if (!strcmp(http_method, "GET")) {
        getRequestedFile(argv[1]);
    } else if (!strcmp(http_method, "POST")) {
        handleFileUpload();
    }
}
