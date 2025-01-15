#include "Client.hpp"

// public
int Client::getEpollfd() {

    return Client::epollfd;
}

void Client::setEpollfd(int epollfd) {

    Client::epollfd = epollfd;
    Client::addOpenFd(epollfd);
}

void Client::setEnvp(const char **envp) {

    Client::envp = envp;
}

void Client::closeFds() {
    if (_clientfd != -1) {
        Client::deleteEvent(_clientfd);
        close(_clientfd);
        _clientfd = -1;
    }
    std::for_each(_cgis.begin(), _cgis.end(), std::mem_fun_ref(&CGI::cleanup));
    _cgis.clear();
    _currptr = _cgis.end();
}

bool Client::isConnClosed() const {
//    time_t diff;
//
//    diff = difftime(time(NULL), _lastresponsetime);
//    std::cout << "diff in time: " << diff << std::endl;
//    std::cout << "\t>>> _iostate = " << _iostate << std::endl;
//    return false;
//    bool io_cond = _iostate == CONN_CLOSED;
//    bool tm_cond = difftime(time(NULL), _lastresponsetime) > TIMEOUT_VAL;
//    std::cout << std::boolalpha << "io_cond: " << io_cond << ", tm_cond: " << tm_cond << std::endl;
//    return io_cond || tm_cond;
    return (_iostate == CONN_CLOSED) || (difftime(time(NULL), _lastresponsetime) > TIMEOUT_VAL);
        //&& _iostate != RECV_CGI && _iostate != SEND_CGI));
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
    httpStatuses[409] = "Conflict";
    httpStatuses[413] = "Content Too Large";
    httpStatuses[414] = "Request-URI Too Long";
    httpStatuses[500] = "Internal Server Error";
    httpStatuses[501] = "Not Implemented";
    httpStatuses[505] = "Version Not Supported";
    return httpStatuses;
}

void Client::setIOState(int state) {

    this->_iostate = state;
}

void Client::setPState(int state) {

    this->_pstate = state;
}
