#include "ConfigFile.hpp"

int main(int argc, char *argv[]) {

    if (argc == 1) {
        argv[1] = const_cast<char *>("default.conf");
    }
    try {
        ConfigFile info(argv[1]);
        info.printServerInfo();
    } catch (const std::exception& e) {
        return std::cerr << e.what() << '\n', 1;
    }
}
