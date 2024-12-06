#include "Client.hpp"

// public
int Client::getEpollfd() {

    return Client::epollfd;
}

void Client::setEpollfd(int epollfd) {

    Client::epollfd = epollfd;
}

void Client::setEnvp(const char **envp) {

    Client::envp = envp;
}

bool Client::isConnClosed() const {

    return io_state == CONN_CLOSED;
}

void Client::setActiveFd(int fd) {

    active_fd = fd;
}

// private
std::map<std::string, std::string> Client::initContentTypes() {
    std::map<std::string, std::string> contentTypes;

    contentTypes["html"] = "text/html";
    contentTypes["css"] = "text/css";
    contentTypes["svg"] = "image/svg-xml";
    contentTypes["ico"] = "image/x-icon";
    return contentTypes;
}

std::map<int, std::string> Client::initHttpStatuses() {
    std::map<int, std::string> httpStatuses;

    httpStatuses[200] = "OK";
    httpStatuses[201] = "Created";
    httpStatuses[400] = "Bad Request";
    httpStatuses[403] = "Forbidden";
    httpStatuses[404] = "Not Found";
    httpStatuses[405] = "Method Not Allowed";
    return httpStatuses;
}

void Client::setIOState(int state) {

    this->io_state = state;
}

void Client::setPState(int state) {

    this->p_state = state;
}
