#include "ClientInfo.hpp"

int ClientInfo::parseStartLine(std::string const& start_line) {
    std::istringstream iss(start_line);
    std::string http_method, http_version;

    if (!(iss >> http_method) || !(iss >> request_uri) || !(iss >> http_version)) {
        return ERROR;
    }
    if (http_method == "GET") {
        this->http_method = GET_METHOD;
    } else if (http_method == "POST") {
        this->http_method = POST_METHOD;
    } else if (http_method == "DELETE") {
        this->http_method = DELETE_METHOD;
    } else {
        ; //error_func(http error status code)
    }
    if (http_version != "HTTP/1.1") {
        ; //error_func(invalid http nbr?)
    }
    return SUCCESS;
}

int ClientInfo::parseHeaders(std::string const& str) {
    size_t pos;
    std::string buf;
    std::stringstream oss(str);

    while (!getline(oss, buf).eof()) {
        if (!oss) {
            oss.exceptions(oss.rdstate());
        } else {
            pos = str.find(':');
            if (pos == std::string::npos) {
                return ERROR;
            }
            headers[buf.substr(0, pos)] = buf.substr(pos + 1);
        }
    }
    return SUCCESS;
}

int ClientInfo::parseMsgBody(std::string const& msg) {
    int bytes;
    int msglen = static_cast<int>(msg.length());

    if (bytes_left != -1) {
        bytes = bytes_left < msglen ? bytes_left : msglen;
    } else {
        bytes = msglen;
    }
    return SUCCESS;
    // need to handle chunked encoding here as well by unchunking it
}

int ClientInfo::parse(std::string const& msg) {
    size_t pos;

    switch (info_type) {
        case START_LINE:
            pos = recvbuf.append(msg).find("\r\n");
            if (pos == std::string::npos) {
                return NEED_MORE;
            } else if (parseStartLine(recvbuf.substr(0, pos)) == ERROR) {
                return ERROR;
            } else {
                recvbuf = recvbuf.substr(pos + 1);
                return SUCCESS;
            }
        case HEADER:
            pos = recvbuf.append(msg).find("\r\n\r\n");
            if (pos == std::string::npos) {
                recvbuf += msg;
                return NEED_MORE;
            } else if (parseHeaders(recvbuf.substr(0, pos)) == ERROR) {
                return ERROR; // ERROR probably needs http status code and descriptive msg
            } else {
                if (headers.count("Content-length")) {
                    bytes_left = atoi(headers["Content-length"].c_str());
                } else {
                    bytes_left = -1; // till EOF
                }
                // parseMsgBody() could be responsible for keeping track of nbr of bytes to read
                msg_body += msg;
                std::copy(recvbuf.begin() + pos + 1, recvbuf.end(), std::back_inserter(msg_body));
                return SUCCESS;
            }
        case MSG_BODY:
            // need to track how much bytes should be read
            std::copy(msg.begin(), msg.end(), std::back_inserter(msg_body));
    }
    return SUCCESS;
}

int ClientInfo::readDataFrom() {
    int bytes, ret;
    char buf[BUFSIZE];

    bytes = recv(sockfd, buf, BUFSIZE, MSG_DONTWAIT);
    if (bytes <= 0) {
        ;
    }
    while (info_type <= MSG_BODY) {
        ret = parse(std::string(buf, bytes));
        if (ret == SUCCESS && info_type != MSG_BODY) {
            ++info_type;
        } else if (ret == ERROR) {
            break ;
        } else {
            break ;
        }
    }
    return 0;
}

std::ostream& operator<<(std::ostream& os, ClientInfo const& client) {
    os << "ClientInfo: \n";
    os << "sockfd: " << client.sockfd << '\n';
    os << "info_type: " << client.info_type << '\n';
    os << "bytes_left: " << client.bytes_left << '\n';
    os << "http_method: " << client.http_method << '\n';
    os << "send_buf: " << client.sendbuf << '\n';
    os << "recv_buf: " << client.recvbuf << '\n';
    os << "msg_body: " << client.msg_body << '\n';
    os << "start_line: " << client.start_line << '\n';
    for (std::map<std::string, std::string>::const_iterator it
        = client.headers.begin(); it != client.headers.end(); ++it) {
        os << it->first << ": " << it->second << '\n';
    }
    return os;
}

ClientInfo::ClientInfo(int fd) :
    sockfd(fd), info_type(0), bytes_left(0), http_method(START_LINE) {}

//ClientInfo::ClientInfo() {}
//ClientInfo::ClientInfo(ClientInfo const&);
//ClientInfo& ClientInfo::operator=(ClientInfo const&) { return *this; }
