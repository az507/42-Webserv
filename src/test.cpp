#include "../include/ConfigFile.hpp"


int main() {
    std::vector<ServerInfo> servers;

    servers.push_back((ServerInfo){});
    servers.begin()->sockfd = 2;
    servers.begin().operator->();

    //std::find_if(servers.begin(), servers.end(), std::bind2nd(std::equal_to<int>( , 2)));

    for (std::vector<ServerInfo>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
        if (std::equal_to<int>(it->sockfd, 2)) {
            break ;
        }
    }
}
