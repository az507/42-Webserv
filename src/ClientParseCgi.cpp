#include "Client.hpp"

void Client::parseCgiOutput(const char *buf, size_t bytes) {
    int res;

    assert(p_state != ERROR);
    assert(io_state == RECV_CGI);
    if (p_state == MSG_BODY && unchunk_flag) {
        recvbuf.append(buf, bytes);
    } else {
        if (p_state == MSG_BODY && track_length) {
            bytes = std::min(bytes_left, bytes);
        }
        msg_body.append(buf, bytes);
    }
    std::cout << buf << '\n';
    do {
        switch (p_state) {
            case START_LINE:    res = ignoreStartLine(); continue ;
            case HEADERS:       res = parseCgiHeaders(bytes); continue ;
            case MSG_BODY:      res = parseMsgBody(bytes); continue ; // assumes CGI output always have msg body
            case FINISHED:      setIOState(SEND_HTTP);
                                close(active_fd);
                                std::swap(active_fd = -1, passive_fd);
                                send_it = msg_body.begin();
                                send_ite = msg_body.end();
                                std::cout << "msg_body.length(): " << msg_body.length() << std::endl;
            case ERROR:         break ;
            default:            std::terminate();
        }
    }
    while (res);
}

int Client::ignoreStartLine() {
    size_t pos;

    pos = msg_body.find("\r\n");
    if (pos == std::string::npos) {
        return 0;
    } else {
        setPState(HEADERS);
        return 1;
    }
}

int Client::parseCgiHeaders(size_t& bytes) {
    std::string buf;
    size_t pos, pos1;
    std::istringstream iss;
    std::map<std::string, std::string> headers;
    static const std::string delim = "\r\n\r\n";

    pos = msg_body.find(delim);
    if (pos == std::string::npos) {
        return 0;
    }
    iss.str(msg_body.substr(0, pos));
    while (!getline(iss, buf).eof()) {
        if (!iss) {
            iss.exceptions(iss.rdstate());
        } else {
            pos1 = buf.find(':');
            if (pos1 != std::string::npos) {
                //headers[recvbuf.substr(0, pos1)] = recvbuf.substr(pos1 + 1);
                headers[msg_body.substr(0, pos1)] = msg_body.substr(pos1 + 1);
            }
        }
    }
    //assert(recvbuf.empty());
    assert(!track_length && !unchunk_flag);
    if (!configureIOMethod(headers)) {
        return -1;
    }
    assert(msg_body.length() >= delim.length() + pos);
    if (track_length) {
        bytes = msg_body.length() - delim.length() - pos;
    } else if (unchunk_flag) {
        recvbuf.append(msg_body.begin() + pos + delim.length(), msg_body.end());
        msg_body.erase(pos + delim.length());
    }
    setPState(MSG_BODY);
    return 1;
}
