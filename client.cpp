#include "includes/header.hpp"

int	main(void)
{
	int	err, sockfd;
	struct addrinfo	hints, *res;
	std::string	message("abc123");
	char	*buffer = new char[MAX_BUFLEN]();

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((err = getaddrinfo("127.0.0.1", "3490", &hints, &res)) != 0)
		return std::cerr << "getaddrinfo error: " << gai_strerror(err) <<'\n', 1;
	if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
		return perror("socket"), 1;
	if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1)
		return perror("connect"), 1;
	while (1)
	{
		memset(buffer, 0, sizeof(char) * MAX_BUFLEN);
//		if (send(sockfd, message.c_str(), message.size(), 0) == -1)
//			return perror("send"), 1;
//		if (recv(sockfd, buffer, MAX_BUFLEN - 1, 0) == -1)
//			return perror("recv"), 1;
		send(sockfd, message.c_str(), message.size(), 0);
		recv(sockfd, buffer, MAX_BUFLEN - 1, 0);
		std::cout << "received from server: " << buffer << std::endl;
		sleep(1);
	}
	close(sockfd);
	freeaddrinfo(res);
	delete [] buffer;
}
