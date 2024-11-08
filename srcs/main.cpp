#include "webserv.hpp"

int parseConfigFile(std::map<std::string, std::vector<std::string> >& vars, const char *filename) {
    size_t pos;
    std::string line;
    std::ifstream infile(filename);

    if (!infile.is_open()) {
        return perror(filename), -1;
    }
    while (1) {
        if (getline(infile, line).eof()) {
            break ;
        } else if (infile.bad() || infile.fail()) {
            return std::cerr << "Critical error occured\n", -1;
        }
        pos = line.find('=');
        if (pos == std::string::npos) {
            return std::cerr << "No equal sign found\n", -1;
        }
        vars[line.substr(0, pos)].push_back(line.substr(pos + 1));
    }
    return 0;
}

//void generateServInfo(std::vector<ServerInfo>& servers, std::map<std::string, std::string>& vars) {
//
//}

int parseDirectives(std::vector<ServerInfo>& servers, std::map<std::string, std::vector<std::string> >& vars) {
    size_t pos;
    std::string name, addr, port;
    std::map<int, Directive> table = {
        { ADDR, { "listen", &nil_handler, &open_sockets } }
    };

    for (int i = 0; i <= LISTEN; ++i) {
        Directive& info = table[i];

        if (!vars.count(info.name)) {
            if (info.handler(servers.
            return std::cerr << "No " << name << " directives\n", -1;
        }
        for (std::vector<std::string>::const_iterator it = vars[name].begin(); it != vars[name].end(); ++it) {
            pos = it->find(':');
            if (pos == std::string::npos) {
                port = *it;
            } else {
                addr = it->substr(0, pos);
                port = it->substr(pos + 1);
            }
            if (!addr.empty()) {
                addr.clear();
            }
            open_sockets
        }
    }
}

void print_pair(std::pair<std::string, std::string> const& pair) {

    std::cout << pair.first << '=' << pair.second << std::endl;
}

int main(int argc, char *argv[]) {
    std::vector<ServerInfo> servers;
    std::map<std::string, std::string> vars;

    if (argc != 2 || !argv[1][0]) {
        return std::cerr << "Usage: " << argv[0] << " <filename>\n", 1;
    }
    if (parseConfigFile(vars, argv[1]) == -1) {
        return 1;
    }
    std::for_each(vars.begin(), vars.end(), print_pair);
}
