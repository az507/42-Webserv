#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#define handle_error(err) \
    do { perror(err); exit(EXIT_FAILURE); } while (0);

std::string getUploadFilepath(const char *dirname) {
    //static uint64_t file_idx = 0;

    //return (std::ostringstream(dirname) << "/entry " << file_idx++).str();
    assert(dirname != NULL);
    (void)dirname;
    //return std::string(dirname) + "/entry0";
    return dirname;
}

std::streamsize getFileLength(const char *filename) {
    return std::ifstream(filename, std::ios::ate | std::ios::binary).tellg();
}

void handleFileUpload() {
    struct stat statbuf;
    std::ofstream outfile;
    const char *upload_dir;
    static const char *dirname = "uploads";

    upload_dir = std::getenv("UPLOAD_DIR");
    if (!upload_dir && stat(dirname, &statbuf) == -1) {
        if (mkdir(dirname, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1) {
            handle_error("mkdir");
        }
        upload_dir = dirname;
    }
    std::cerr << "upload_dir: " << upload_dir << '\n';
    assert(upload_dir != NULL);
    outfile.open(getUploadFilepath(upload_dir).c_str());
    if (!outfile.is_open()) {
        handle_error("open outfile");
    }
    char buf[1024];
    int bytes;
    std::string strbuf;
    while (1) {
        bytes = recv(STDIN_FILENO, buf, 1023, MSG_DONTWAIT);
        if (bytes <= 0) {
            perror("in cgi, recv"); break ;
        }
        strbuf.append(buf, bytes);
    }
    std::copy(strbuf.begin(), strbuf.end(), std::ostreambuf_iterator<char>(outfile));
    std::string msg = "File uploaded successfully\r\n";
    //std::copy(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>(), std::ostreambuf_iterator<char>(outfile));
    std::cout << "HTTP/1.1 200 OK\r\n";
    std::cout << "Content-Length: " << msg.length() << "\r\n";
    std::cout << "Content-Type: text/plain\r\n";
    std::cout << "Connection: keep-alive\r\n\r\n";
    std::cout << msg << "\r\n";
}

void getRequestedFile(const char *filename) {
    std::ifstream infile(filename);

    if (!infile.is_open()) {
        handle_error("open infile");
    }
    std::cout << "HTTP/1.1 200 OK\r\n";
    std::cout << "Content-Length: " << getFileLength(filename) << "\r\n";
    //std::cout << "Content-Type: " << getFileExtension(filename) << "\r\n";
    std::cout << "Connection: keep-alive\r\n\r\n";
    std::copy(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>(), std::ostream_iterator<char>(std::cout));
}

int main(int argc, char *argv[]) {
    char *_httpmethod;

    //try {
        assert(argc >= 2);
        assert(argv[1] != NULL);
        _httpmethod = std::getenv("REQUEST_METHOD");
        assert(_httpmethod != NULL);
        //assert(false);
        if (!strcmp(_httpmethod, "GET")) {
            getRequestedFile(argv[1]);
        } else if (!strcmp(_httpmethod, "POST")) {
            handleFileUpload();
        }
//    }
//    catch (std::exception const& e) {
//        std::cerr << e.what() << '\n';
//    }
}
