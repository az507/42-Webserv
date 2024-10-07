#include <list>
#include <iostream>
#include <string>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define BUFFER_SIZE 100
#define CLIENTS_MAX 10

struct	parser {
//	std::list<std::pair<char *, size_t> > input;
//	std::list<std::pair<char *, size_t> > lastinput;
//	std::list<std::pair<char *, size_t> > output;
	int (*func)(struct parser *p);
};

void	ft_perror(const char *err) {
	if (errno) {
		std::cerr << err << ": " << strerror(errno) << '\n';
		errno = 0;
	}
}

//void	parse(std::list<std::pair<char *, size_t> >& buflist) {
//
//}

//class HttpRequest {
//	public:
//		int	method, version;
//		char *targetUri;
//		std::map<std::string, std::string> headers;
//};

// read_start_line
// read_headers
// read_message_body
void read_from_client(int fd) {
	std::list<std::pair<char *, size_t> > buflist;
	ssize_t bytes_recv;
	char *buf;
	while (1) {
		buf = new char[BUFFER_SIZE + 1]();
		bytes_recv = recv(fd, buf, BUFFER_SIZE, MSG_DONTWAIT);
		if (bytes_recv <= 0) { // client closed the socketfd || this recv op would have blocked if MSG_DONTWAIT was not set
			delete [] buf;
			return ;
		}
	}
	for (std::list<std::pair<char *, size_t> >::iterator it = buflist.begin(); it != buflist.end(); ++it) {
		std::cout << it->first;
		delete [] it->first;
	}
	std::cout << std::endl;
}

int	main(void) {
	struct addrinfo hints, *res;
	struct epoll_event ev, events[CLIENTS_MAX];
	int err, sockfd, epollfd, nfds, connfd;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	err = getaddrinfo(NULL, "80", &hints, &res);
	if (err) {
		return std::cerr << "getaddrinfo: " << gai_strerror(err) << '\n', 1;
	}
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd == -1) {
		return ft_perror("socket"), 1;
	}
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
		return ft_perror("bind"), 1;
	}
	if (listen(sockfd, CLIENTS_MAX) == -1) {
		return ft_perror("listen"), 1;
	}
	epollfd = epoll_create(1);
	if (epollfd == -1) {
		return ft_perror("epoll_create"), 1;
	}
	ev.events = EPOLLIN;
	ev.data.fd = sockfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
		return ft_perror("epoll_ctl"), 1;
	}
	while (1) {
		nfds = epoll_wait(epollfd, events, CLIENTS_MAX, -1);
		if (nfds == -1) {
			return ft_perror("epoll_wait"), 1;
		}
		for (int i = 0; i < nfds; ++i) {
			if (events[i].data.fd == sockfd) {
				connfd = accept(sockfd, NULL, NULL);
				if (connfd == -1) {
					return ft_perror("accept"), 1;
				}
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = connfd;
				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1) {
					return ft_perror("epoll_ctl"), 1;
				}
			} else {
				read_from_client(events[i].data.fd);
			}
		}
	}
	freeaddrinfo(res);
	close(sockfd);
	close(epollfd);
}
