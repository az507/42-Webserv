#include "webserv.hpp"

void	read_fd(int fd) {
	ssize_t	bytes_recv;
	char buffer[BUFFER_SIZE];
	std::string message("This is a message.\n");
	std::stringstream strbuf;

	for (int i = 1; ; ++i) {
		memset(buffer, 0, BUFFER_SIZE);
		bytes_recv = recv(fd, buffer, BUFFER_SIZE - 1, MSG_DONTWAIT);
		if (bytes_recv <= 0) {
			perror("recv");
			send(fd, message.c_str(), message.size(), MSG_DONTWAIT);
			close(fd);
			break ;
		}
		//strbuf << buffer;
		//std::cout << "bytes_recv = " << bytes_recv << std::endl;
		std::cout << buffer;
	}
	std::cout << std::endl;
	//std::cout << "strbuf.str() = " << strbuf.str() << std::endl;
}

int	main(void) {
	struct addrinfo	hints, *res;
	struct epoll_event	ev, events[MAX_EVENTS];
	int	err, sockfd, optval = 1, epollfd, nfds, newsockfd;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	err = getaddrinfo(NULL, "8080", &hints, &res);
	if (err) {
		return std::cerr << "getaddrinfo error: " << gai_strerror(err) << '\n', 1;
	}
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd == -1) {
		return perror("sockfd"), 1;
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
		return perror("setsockopt"), 1;
	}
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
		return perror("bind"), 1;
	}
	if (listen(sockfd, 5) == -1) {
		return perror("listen"), 1;
	}
	epollfd = epoll_create(1);
	if (epollfd == -1) {
		return perror("epoll_create"), 1;
	}
	ev.events = EPOLLIN;
	ev.data.fd = sockfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
		return perror("1) epoll_ctl"), 1;
	}
	while (1) {
		std::cout << "One iteration" << std::endl;
		nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			return perror("epoll_wait"), 1;
		}
		for (int i = 0; i < nfds; ++i) {
			if (events[i].data.fd == sockfd) {
				newsockfd = accept(sockfd, NULL, NULL);
				if (newsockfd == -1) {
					return perror("accept"), 1;
				}
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = newsockfd;
				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, newsockfd, &ev) == -1) {
					return perror("2) epoll_ctl"), 1;
				}
			} else {
				read_fd(events[i].data.fd);
			}
		}
	}
	freeaddrinfo(res);
	close(sockfd);
	close(epollfd);
}
