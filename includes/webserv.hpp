#ifndef WEBSERV_HPP
#define WEBSERV_HPP

# include <map>
# include <list>
# include <vector>
# include <iostream>
# include <sstream>
# include <fstream>
# include <string>
# include <algorithm>
# include <cstring>
# include <cstdlib>
# include <cstdio>
# include <cassert>
//# include <arpa/inet.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/wait.h>
# include <sys/socket.h>
# include <sys/epoll.h>
# include <netdb.h>
# include <unistd.h>
# include <errno.h>
# include "ServerInfo.hpp"

# define BUF_SIZE 1024
# define MAX_EVENTS 10
# define LISTEN_BACKLOG 50
# define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

#endif
