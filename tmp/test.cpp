#include <list>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <exception>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cassert>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
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
	assert(errno);
	std::cerr << err << ": " << strerror(errno) << '\n';
	errno = 0;
}

class	StrFunctor {
	public:
		StrFunctor(const char *s) : s1(s) {}
		bool operator()(const char *s2) {
			return !strcmp(s1, s2);
		}
	private:
		const char *s1;
};

/*
// theres a edge case to consider where half of the string is in 1 buf, the other half in the next buf
// if the buffer_size is too small/str too big, then the string we r looking for could span multiple bufs
// simple way would be to just strjoin all the buffers into 1 giant buffer, then parse from there
std::pair<std::list<char *>::iterator, char *>
find_first_of(std::list<char *>& bufs, const char *start, const char *target) {
	char *res;
	StrFunctor functor(start);
	//for (std::list<char *>::iterator it = std::find_if(bufs.begin(), bufs.end(), functor); it != bufs.end(); ++it) {
	for (std::list<char *>::iterator it = bufs.begin(); it != bufs.end(); ++it) {
		res = strstr(*it, target);
		if (res) {
			return (std::make_pair(it, res));
		}
	}
	return (std::make_pair(bufs.end(), (char *)NULL));
}

std::pair<char *, bool>
find_delim(const char *buf, const std::string& delim) {
	const char *res, tmp = buf;
	for (std::string::const_iterator it = delim.begin(); it != delim.end(); ++it) {
		res = strchr(tmp, *it);
		if (!res) {
			if (!*(tmp + 1)) {
				return (std::make_pair(tmp, false));
			} else {
				return (std::make_pair((char *)NULL, false));
			}
		} else {
			tmp = res;
		}
	}
	return (std::make_pair(tmp, true));
}
*/

void send_to_client(int fd, char *envp[]) {
	(void)envp;
	/*
	int pipefds[2];
	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, pipefds) == -1) {
		perror("socketpair");
	}
	pid_t pid = fork();
	if (pid == -1) {
		perror("fork");
	} else if (!pid) {
		close(pipefds[1]);
		dup2(pipefds[0], STDIN_FILENO);
		dup2(pipefds[0], STDOUT_FILENO);
		close(pipefds[0]);
		//const char *argv[] = { "/home/achak/Documents/ws/first.pl", NULL };
		const char *argv[] = { "/home/achak/Documents/ws/print", NULL };
		execve(argv[0], (char *const*)argv, envp);
	} else {
		close(pipefds[0]);
		std::list<char *> buflst;
		char *buf = new char[201]();
		ssize_t bytes;
		while (1) {
			bytes = recv(pipefds[1], buf, 200, 0);
			if (bytes <= 0) {
				perror("recv");
				delete [] buf;
				break ;
			}
			std::cout << "bytes: " << bytes << std::endl;
			std::cout << "buf: " << buf << std::endl;
			bytes = send(fd, buf, bytes, 0);
			std::cout << "bytes sent = " << bytes << std::endl;
		}
		close(pipefds[1]);
		waitpid(pid, NULL, 0);
		for (std::list<char *>::iterator it = buflst.begin(); it != buflst.end(); ++it)
		{
			bytes = send(fd, *it, 200, 0);
			delete [] *it;
		}
	}
	*/
	std::ifstream infile("index.html");
	if (!infile.is_open()) {
		perror("index.html");
	}
	std::string msg, header("HTTP/1.1 200 OK\n\n");
	send(fd, header.c_str(), header.size(), 0);
	while (getline(infile, msg)) {
		if (!infile.eof()) {
			msg.append(1, '\n');
		}
		//std::cout << msg;
		send(fd, msg.c_str(), msg.size(), 0);
	}
}

// read_start_line
// read_headers
// read_message_body
void	read_from_client(int fd) {
	int i = 0;
	size_t len1, len2;
	ssize_t bytes;
	char *buf = NULL, *newbuf, *tmp;
	while (1) {
		newbuf = new char[BUFFER_SIZE + 1];
		bytes = recv(fd, newbuf, BUFFER_SIZE, MSG_DONTWAIT);
		if (bytes <= 0) { // client closed the socketfd
				//|| this recv op would have blocked if MSG_DONTWAIT was not set
			delete [] buf;
			delete [] newbuf;
			if (!bytes) {
				close(fd);
				//throw 1;
			}
			break ;
		}
		if (!buf) {
			buf = newbuf;
			buf[bytes] = '\0';
		} else {
			len1 = strlen(buf);
			len2 = strlen(newbuf);
			tmp = new char[len1 + len2 + 1];
			strncpy(tmp, buf, len1);
			strncpy(tmp + len1, newbuf, len2);
			delete [] buf;
			delete [] newbuf;
			tmp[len1 + len2] = '\0';
			buf = tmp;
		}
		std::cout << buf << std::endl;
		++i;
	}
	std::cout << std::endl;
	std::cerr << "Func end\n";
}

/*
int	cgi_script(int fd, char *envp[]) {
	pid_t pid = fork();
	std::cout << "in cgi_script func" << std::endl;
	(void)fd;
	if (!pid) {
		const char *arr[] = { "./script.pl", NULL };
		execve(arr[0], (char *const *)arr, envp);
		return 0;
	} else {
		waitpid(pid, NULL, 0);
		return 1;
	}
}
*/

bool	read_from_file(const char *filename) {
	std::ifstream file(filename);
	std::string strbuf;
	if (!file.is_open()) {
		std::cerr << "could not open file " << filename << '\n';
		return false;
	}
	while (file >> strbuf) {
		std::cout << strbuf << std::endl;
	}
	return true;
}

#include <arpa/inet.h>
int	main(int argc, char *argv[], char *envp[]) {
	(void)argc;
	(void)argv;
	(void)envp;
	if (argc != 2 || !argv[1][0]) {
		std::cerr << "Usage: " << argv[0] << " <config file>\n";
		return 1;
	}
//	if (!read_from_file(argv[1])) {
//		return 1;
//	}
//	return 0;
	std::vector<int>	fd_vec;
	struct addrinfo hints, *res;
	struct epoll_event	ev, events[CLIENTS_MAX];
	int err, sockfd, optval = 1, epollfd, nfds, connfd;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	err = getaddrinfo(NULL, "10000", &hints, &res);
	if (err) {
		return std::cerr << "getaddrinfo: " << gai_strerror(err) << '\n', 1;
	}
//	for (struct addrinfo *p = res; p; p = p->ai_next) {
//		std::cout << "p->ai_canonname: " << p->ai_canonname << std::endl;
//	}
//	freeaddrinfo(res);
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd == -1) {
		return ft_perror("socket"), 1;
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
		return ft_perror("setsockopt"), 1;
	}
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
		return ft_perror("bind"), 1;
	}
	if (listen(sockfd, 5) == -1) {
		return ft_perror("listen"), 1;
	}
	epollfd = epoll_create(1);
	if (epollfd == -1) {
		return ft_perror("epoll_create"), 1;
	}
	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.fd = sockfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
		return ft_perror("epoll_ctl"), 1;
	}
	while (1) {
		//std::cout << "Before" << std::endl;
		nfds = epoll_wait(epollfd, events, CLIENTS_MAX, -1);
		if (nfds == -1) {
			return ft_perror("epoll_wait"), 1;
		}
		//std::cout << "After, nfds = " << nfds << std::endl;
		for (int i = 0; i < nfds; ++i) {
			if (events[i].data.fd == sockfd) {
				connfd = accept(sockfd, NULL, NULL);
				if (connfd == -1) {
					return ft_perror("accept"), 1;
				}
				fd_vec.push_back(connfd);
				ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
				ev.data.fd = connfd;
				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1) {
					return ft_perror("epoll_ctl"), 1;
				}
				fd_vec.push_back(connfd);
			} else {
				read_from_client(events[i].data.fd);
				send_to_client(events[i].data.fd, envp);
				freeaddrinfo(res);
				close(sockfd);
				close(epollfd);
				for (std::vector<int>::iterator it = fd_vec.begin();
					it != fd_vec.end(); ++it) {
					close(*it);
				}
				return 0;
//				if (!cgi_script(events[i].data.fd, envp))
//					return 0; // child process
			}
		}
	}
	freeaddrinfo(res);
	close(sockfd);
	close(epollfd);
}
