#include "webserv.hpp"

/*
const std::vector<std::vector<std::string> > tokens = {
	{
		"server",
		"error_page",
		"max_client_body_size",
	},
	{
		"listen",
		"server_name",
		"location",
	},
	{
		"root",
		"index",
		"autoindex",
		"limit_except",
		"
};
*/

struct LocationInfo {
	bool dir_list;
	std::string root;
	std::string index;
	std::string prefix_str;
	std::string redir_path;
	std::vector<std::string> http_methods;
};

struct AddrInfo {
	int sockfd;
	std::string port;
	std::string ipaddr;
}

struct ServerInfo {
	ServerInfo() { memset(this, 0, sizeof(*this)); }
	std::vector<AddrInfo> addrs;
	std::vector<LocationInfo> routes;
	std::vector<std::string> server_names;
	static int max_clients;
	static std::vector<std::string> error_pages;
};

int parseLocationInfo(ServerInfo& data, std::ifstream& file, std::string& arg) {
	LocationInfo info;
	std::stringstream ss;
	std::string line, token, arg;

	info.dir_list = false;
	while (getline(file, line)) {
		ss.str(line);
		if (ss >> token) {
			if (token == "}")
				break ;
			ss.str(ss.str().c_str() + token.length() + 1);
			if (!(ss >> arg))
				return std::cerr << "no unexpected argument for token: " << token << '\n', 0;
			if (token == "root") {
				info.root = arg;
			} else if (token == "index") {
				info.index = arg;
			} else if (token == "return") {
				info.redir_path = arg;
			} else if (token == "autoindex") {
				if (arg == "on")
					info.dir_list = true;
			} else if (token == "limit_except") {

			} else {
				return std::cerr << "Unexpected token: " << token << '\n', 0;
			}
		} else {
			return std::cerr << "\terr in parseLocationInfo\n", 0;
		}
	}
	data.routes.push_back(info);
	return 1;
}

int parseServerInfo(std::vector<ServerInfo>& servinfo, std::ifstream& file) {
	ServerInfo data;
	std::stringstream ss;
	std::string line, token, arg;

	// once we see the closing '}' brace, bail out of this func early
	while (getline(file, line)) {
		ss.str(line);
		if (ss >> token) {
			if (token == "}")
				break ;
			ss.str(ss.str().c_str() + token.length() + 1);
			if (!(ss >> arg))
				return std::cerr << "no unexpected argument for token: " << token << '\n', 0;
			if (token == "location") {
				if (ss >> token && token == "{" && !parseLocationInfo(data, file, arg)) {
					continue ;
				} else {
					return std::cerr << "Error in location context\n", 0;
				}
			} else if (token == "listen") {
				data.addrs.push_back((AddrInfo){0, arg, ""});
			} else if (token == "server_name") {
				data.server_names.push_back(arg);
			} else {
				return std::cerr << "Unexpected token: " << token << '\n', 0;
			}
		} else {
			return std::cerr << "\terr in parseServerInfo\n", 0;
		}
	}
	servInfo.push_back(data);
	return 1;
}

// 2 levels deep, 1st) parse server stuff, which can go to 2nd) parse location stuff
int parseConfigFile(std::vector<ServerInfo>& servinfo, const char *filename) {
	std::stringstream ss;
	std::string line, token, arg;
	std::ifstream file(filename);

	if (!file.is_open())
		return perror(filename), 0;
	while (getline(file, line)) {
		ss.str(line);
		if (ss >> token) {
			ss.str(ss.str().c_str() + token.length() + 1);
			if (!(ss >> arg))
				return std::cerr << "no argument found for token: " << token << '\n', 0;
			if (token == "server") {
				if (arg != "{") {
					return std::cerr << "No opening '{' brace for server context\n", 0;
				} else if (!parseServerInfo(servinfo, file)) {
					return 0;
				}
			} else if (token == "error_page") {
				ServerInfo::error_pages.push_back(arg);
			} else if (token == "max_client_body_size") {
				ServerInfo::max_clients = atoi(arg.c_str());
			} else {
				return std::cerr << "Unexpected token: " << token << '\n', 0;
			}
		} else {
			return std::cerr << "\terr\n", 0;
		}
		ss.str("");
		ss.clear();
	}
	return 1;
}
