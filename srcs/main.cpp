#include "webserv.hpp"

void	read_fd(int fd) {

	ssize_t	bytes_recv;
	char	buffer[BUFFER_SIZE];

	while (1) {
		memset(buffer, 0, BUFFER_SIZE);
		bytes_recv = recv(fd, buffer, BUFFER_SIZE - 1);
		if (bytes_recv <= 0) {
			perror("recv");
			close(rd);
			return ;
		}
		std::cout << "buffer:	" << buffer << "\n\n" << std::endl;
	}
}

int	main(void) {
	struct addrinfo		hints;
	struct epoll_event	ev, events[MAX_EVENTS];
	int			err, sockfd, epollfd, nfds, newsockfd;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	err = getaddrinfo("NULL", 80, &hints, &res);
	if (err) {
		return std::cerr << "getaddrinfo error: " << gai_strerror(err) << '\n', 1;
	}
	sockfd = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
	if (sockfd == -1) {
		return perror("sockfd"), 1;
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
	if (epoll_ctl(epollfd, sockfd, EPOLL_CTL_ADD, &ev) == -1) {
		return perror("epoll_ctl"), 1;
	}
	while (1) {
		if ((nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1)) == -1) {
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
				if (epoll_ctl(epollfd, newsockfd, EPOLL_CTL_ADD, &ev) == -1) {
					return perror("epoll_ctl"), 1;
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
