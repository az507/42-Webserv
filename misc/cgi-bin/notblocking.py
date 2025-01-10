-----------------------------1520739193976474588769191324
Content-Disposition: form-data; name="fname"

x
-----------------------------1520739193976474588769191324
Content-Disposition: form-data; name="uploaded_file"; filename="file.cpp"
Content-Type: text/x-c++src

#include <fstream>
#include <iostream>
#include <iterator>
#include <algorithm>

// may not be reliable way of telling file length, can use stat() instead?
std::streampos getFileLength(const char *filename) {
    return std::ifstream(filename, std::ios::ate | std::ios::binary).tellg();
}

int main(int argc, char *argv[], char *envp[]) {
    std::ifstream infile;

    (void)envp;
    if (argc == 2) {
        infile.open(argv[1]);
        if (!infile.is_open()) {
            return 1;
        }
        std::cout << "HTTP/1.1 200 OK\r\n";
        std::cout << "Content-Length: " << getFileLength(argv[1]) << "\r\n";
        std::cout << "Content-Type: text/plain\r\n";
        std::cout << "Connection: keep-alive\r\n\r\n";
        std::copy(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>(), std::ostream_iterator<char>(std::cout));
    }
}

-----------------------------1520739193976474588769191324--
