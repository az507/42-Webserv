#include "ConfigFile.hpp"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        return std::cerr << "Usage: " << argv[0] << ": <filename>\n", 1;
    }
    try {
        ConfigFile info(argv[1]);
        info.printServerInfo();
    } catch (const std::exception& e) {
        return std::cerr << e.what() << '\n', 1;
    } catch (...) {
        return std::cerr << "exception caught\n", 1;
    }
}
