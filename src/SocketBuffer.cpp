#include "SocketBuffer.hpp"

SocketBuffer::SocketBuffer(int fd) : sockfd(fd), recv_offset(0), send_offset(0) {}

void SocketBuffer::recvData() {
    int bytes;
    char buf[BUFSIZE];

    bytes = recv(sockfd, buf, BUFSIZE, MSG_DONTWAIT);
    switch (bytes) {

        case 0:         ; break ; // client has closed connection
        case -1:        throw std::exception(); // not sure when this would happen
        default:        recvbuf.append(buf, bytes);
    }
}

void SocketBuffer::sendData() {
    int bytes;

    bytes = send(sockfd, sendbuf.c_str(), sendbuf.length() - send_offset, MSG_DONTWAIT);
    switch (bytes) {
        case 0:         ; break ;
        case -1:        throw std::exception();
        default:        
    }
}
