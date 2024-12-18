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

void Client::closeFds() {

    //throw std::exception();
    if (active_fd > 0) {
        close(active_fd);
    }
    if (passive_fd > 0) {
        close(passive_fd);
    }
    active_fd = passive_fd = -1;
}

bool Client::isConnClosed() const {
//    time_t diff;
//
//    diff = difftime(time(NULL), client_conn_time);
//    std::cout << "diff in time: " << diff << std::endl;
//    std::cout << "\t>>> io_state = " << io_state << std::endl;
    return (io_state == CONN_CLOSED) || (difftime(time(NULL), client_conn_time) > TIMEOUT_VAL);
        //&& io_state != RECV_CGI && io_state != SEND_CGI));
}

void Client::setActiveFd(int fd) {

    active_fd = fd;
}

std::pair<int, int> Client::getAllFds() const {
    
    return std::make_pair(active_fd, passive_fd);
}

// private
std::map<std::string, std::string> Client::initContentTypes() {
    std::map<std::string, std::string> contentTypes;

    contentTypes["html"] = "text/html";
    contentTypes["css"] = "text/css";
    contentTypes["svg"] = "image/svg-xml";
    contentTypes["ico"] = "image/x-icon";
    contentTypes["png"] = "image/png";
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
