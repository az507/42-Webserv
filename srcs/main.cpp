#include "webserv.hpp"

int	setup_socket(const char *node, const char *service) {
	struct addrinfo hints, *res;
	int err, sockfd, optval = 1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((err = getaddrinfo(node, service, &hints, &res))) {
		return std::cerr << "getaddrinfo: " << gai_strerror(err) << '\n', -1; // critical
	}

	for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("socket");
			continue ;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
			handle_error("setsockopt"); // critical
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("bind");
			close(sockfd);
			continue ;
		}
		break ;
	}
	freeaddrinfo(res);
	return sockfd;
}

char *read_http_request(int fd) {
	std::ptrdiff_t offset;
	std::list<std::pair<char *, ssize_t> > buflst;
	char *buf;
	ssize_t bytes;

	while (1) {
		offset = 0;
		buf = new char[BUFFER_SIZE + 1]();
		while (offset < BUFFER_SIZE) {
			bytes = recv(fd, buf + offset, BUFFER_SIZE - offset, MSG_DONTWAIT);
			if (bytes <= 0) {
				if (!bytes) { // connection closed by client
					close(fd);
					delete [] buf;
					for (std::list<std::pair<char *, ssize_t> >::iterator it = buflst.begin(); it != buflst.end(); ++it) {
						delete [] it->first;
					}
					return NULL;
				}
				break ;
			}
			offset += bytes;
			buflst.push_back(std::make_pair(buf, bytes));
		}
		if (bytes == -1) {
			break ;
		}
	}
	offset = bytes = 0;
	for (std::list<std::pair<char *, ssize_t> >::const_iterator it = buflst.begin(); it != buflst.end(); ++it) {
		bytes += it->second;
	}
	buf = new char[bytes + 1];
	for (std::list<std::pair<char *, ssize_t> >::iterator it = buflst.begin(); it != buflst.end(); ++it) {
		strncpy(buf + offset, it->first, it->second);
		delete [] it->first;
	}
	buf[bytes] = '\0';
	return buf;
}

struct HttpRequest {
	char *buf, *msg_body;
	std::vector<std::string> info; // info[0] = method, [1] = target, [2] = http version no.
	std::map<std::string, std::string> headers;
};

HttpRequest parse_http_request(char *buffer) {
	bool first = true;
	char *startl = buffer, *endl, *delim;
	std::string buf;
	HttpRequest data;
	std::stringstream ss;
//	std::vector<std::string> info;
//	std::map<std::string, std::string> headers;

	delim = strstr(buffer, "\r\n\r\n");
	assert(delim != NULL);
	*(delim + 3) = '\0';
	while (1) {
		endl = strchr(startl, '\n');
		if (!endl)
			break ;
		*endl = '\0';
		ss.str(startl);
		assert(ss >> buf);
		if (first) {
			do {
				data.info.push_back(buf);
				ss.str(ss.str().c_str() + buf.length() + 1);
			} while (ss >> buf);
			first = false;
		} else {
			data.headers[buf] = std::string(ss.str().c_str() + buf.length() + 1);
		}
		ss.str("");
		ss.clear();
		startl = endl + 1;
	}
//	for (std::vector<std::string>::const_iterator it = data.info.begin(); it != data.info.end(); ++it) {
//		std::cout << *it << ' ';
//	}
//	std::cout << std::endl;
//	for (std::map<std::string, std::string>::const_iterator it = data.headers.begin(); it != data.headers.end(); ++it) {
//		std::cout << it->first << ", " << it->second << std::endl;
//	}
	data.buf = buffer;
	data.msg_body = delim + 4;
	return data;
}

// need to figure out which virtual server will serve this request
void send_http_response(int fd, char *envp[]) {
	(void)envp;
	ssize_t bytes;
	std::ptrdiff_t offset = 0;
	std::stringstream ss;
	struct stat statbuf;
	//const char *filename = "/home/achak/Documents/webserv/index.html";
	const char *filename = "/home/achak/index.html";
	char *filebuf;
	std::ifstream infile(filename);

	if (stat(filename, &statbuf) == -1)
		handle_error("stat");
	filebuf = new char[statbuf.st_size + 1]();
	infile.read(filebuf, statbuf.st_size);
	std::cout << "filesize: " << statbuf.st_size << std::endl;
	ss << "HTTP/1.1 200 OK\n\n";
	//ss << "Content-Type: text/html\n\n"; // html forms can fill this line for us alr, including this can cause the browser to mistakenly intepret the html form as text
	ss << filebuf;
//	for (int i = 0; envp[i] != NULL; ++i)
//		ss << envp[i] << '\n';
	while ((size_t)offset < ss.str().length()) {
		bytes = send(fd, ss.str().c_str() + offset, ss.str().length() - offset, MSG_DONTWAIT);
		if (bytes <= 0) {
			assert(bytes != -1);
			if (!bytes) {
				close(fd); // client closed connection
				delete [] filebuf;
				return ;
			}
		}
		offset += bytes;
	}
	delete [] filebuf;
	close(fd);
}

int	main(int argc, char *argv[], char *envp[]) {
	(void)argc;
	(void)argv;
	(void)envp;
	char *request;
	int sockfd, epfd, nfds, connfd;
	struct HttpRequest data;
	struct epoll_event ev, events[MAX_EVENTS];

	if (argc != 2 || !argv[1][0])
		return std::cerr << "Usage: " << argv[0] << " <filename>\n", 1;
	if (!parseConfigFile(argv[1]))
		return 1;
	sockfd = setup_socket(NULL, "8080");
	if (sockfd == -1)
		return 1;
	if (listen(sockfd, LISTEN_BACKLOG) == -1)
		handle_error("listen");
	epfd = epoll_create(1);
	if (epfd == -1)
		handle_error("epoll_create");
	ev.events = EPOLLIN;
	ev.data.fd = sockfd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev) == -1)
		handle_error("epoll_ctl");
	while (1) {
		nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
		if (nfds == -1)
			handle_error("epoll_wait");
		for (int i = 0; i < nfds; ++i) {
			if (events[i].data.fd == sockfd) {
				connfd = accept(sockfd, NULL, NULL);
				if (connfd == -1)
					handle_error("accept");
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = connfd;
				if (epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev) == -1)
					handle_error("epoll_ctl");
			} else {
				request = read_http_request(events[i].data.fd);
				if (!request)
					continue ;
				std::cout << request << "\n\n\n" << std::endl;
				data = parse_http_request(request);
				send_http_response(events[i].data.fd, envp);
				return 0;
			}
		}
	}
}
