/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   chunkedtestscrip.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xzhang <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/13 11:46:00 by xzhang            #+#    #+#             */
/*   Updated: 2025/01/13 11:46:08 by xzhang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include <iostream>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    // Server details
    const char *server_ip = "127.0.0.1";  // Change to your server's IP if needed
    const int server_port = 8080;

    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Set up server address structure
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        return 1;
    }

    // Connect to the server
    if (connect(sock, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return 1;
    }

    // HTTP request to send unchunked data
    std::string body = "hello\nworldxxxxx";
    std::ostringstream request;
    request << "POST /cgi-bin/xx.txt HTTP/1.1\r\n"
            << "Host: " << server_ip << ":" << server_port << "\r\n"
            << "Content-Type: text/plain\r\n"
            << "Content-Length: " << body.size() << "\r\n"
            << "Connection: close\r\n"
            << "\r\n"
            << body;

    // Send the request
    std::string request_str = request.str();
    if (send(sock, request_str.c_str(), request_str.size(), 0) < 0) {
        perror("Send failed");
        close(sock);
        return 1;
    }

    // Receive the response
    char buffer[4096];
    ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (received < 0) {
        perror("Receive failed");
        close(sock);
        return 1;
    }

    // Null-terminate the response and print it
    buffer[received] = '\0';
    std::cout << "Server response:\n" << buffer << std::endl;

    // Close the socket
    close(sock);
    return 0;
}
