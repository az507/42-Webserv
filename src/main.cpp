/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xzhang <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 11:00:02 by xzhang            #+#    #+#             */
/*   Updated: 2024/11/18 11:00:10 by xzhang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include <iostream>  // For std::cerr
#include <cstdlib>   // For std::strtol

// int main(int argc, char* argv[]) {
//     // Default port
//     int port = 8080;

//     // Allow the user to specify a port via command-line arguments
//     if (argc == 2) {
//         char* endptr;
//         long temp_port = std::strtol(argv[1], &endptr, 10); // Convert to long
        
//         // Validate the conversion and range
//         if (*endptr != '\0' || temp_port <= 0 || temp_port > 65535) {
//             std::cerr << "Invalid port number: " << argv[1] << std::endl;
//             return EXIT_FAILURE;
//         }
        
//         port = static_cast<int>(temp_port);
//     } else if (argc > 2) {
//         std::cerr << "Usage: " << argv[0] << " [port]" << std::endl;
//         return EXIT_FAILURE;
//     }

//     // Print server start message
//     std::cout << "Starting server on port " << port << "..." << std::endl;

//     try {
//         Server server(port); // Initialize server
//         server.run();        // Start handling connections
//     } catch (const std::exception& e) {
//         std::cerr << "Error: " << e.what() << std::endl;
//         return EXIT_FAILURE;
//     }

//     return EXIT_SUCCESS;
// }

#include "../include/Server.hpp"
#include <iostream>
#include <vector>
#include <utility>
#include <string>

int main() {
    // Define server configurations as a vector of <IP, Port>
    std::vector<std::pair<std::string, std::string> > servers;
    servers.push_back(std::make_pair("", "8080"));       // All interfaces on port 8080
    servers.push_back(std::make_pair("127.0.0.1", "9090")); // Localhost on port 9090

    // Define the root directory for serving files
    std::string rootdir = "/var/www"; // Change this to the directory you want to serve
    std::map<int, std::string> errorpage;
    errorpage.insert(std::make_pair(404, "/errors/404.html"));
    errorpage.insert(std::make_pair(500, "/errors/500.html"));
    try 
    {
        // Initialize the server with configurations and root directory
        Server server(servers, rootdir);

        // Start the server
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
