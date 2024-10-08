#ifndef WEBSERV_HPP
#define WEBSERV_HPP

# include <iostream>
# include <sstream>
# include <fstream>
# include <string>
# include <cstring>
# include <cstdlib>
# include <cstdio>
# include <arpa/inet.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <sys/socket.h>
# include <sys/epoll.h>
# include <netdb.h>
# include <unistd.h>
# include <errno.h>

# define MAX_EVENTS 10
# define BUFFER_SIZE 100

/*
class HttpPacket {
	public:
		HttpPacket(std::stringstream& strbuf) {
			std::string key, value;
			std::getline(strbuf, this->startLine);
			while (1) {
				if (!std::getline(strbuf, key, " ")
					|| !std::getline(strbuf, value)) {
					throw std::runtime_error("parsing error");
				}
				this->headerFields[key] = value;
			}

		}
	private:
		std::map<std::string, std::string>	headerFields;
		std::string				startLine, messageBody;
		int					messageLen;
};
*/

#endif
