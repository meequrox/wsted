#include <iomanip>
#include <iostream>

#define DEFAULT_PORT 8044

void usage(std::string exe) {
    std::cout << std::left;

    std::cout << "Usage: " << std::endl;
    std::cout << exe << std::setw(32) << " PORT"
              << "use custom port (1024-49151)" << std::endl;
    std::cout << std::setw(32 + exe.size()) << exe << "use default port (" << DEFAULT_PORT << ")"
              << std::endl;
}

int main(int argc, char* argv[]) {
    int port;

    if (argc == 1) {
        port = DEFAULT_PORT;
    } else if (argc == 2) {
        auto newPort = std::atoi(argv[1]);
        port = newPort >= 1024 && newPort <= 49151 ? newPort : DEFAULT_PORT;
    } else {
        std::cout << "Too many arguments" << std::endl << std::endl;

        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    std::cout << "wsted server on 0.0.0.0:" << port << std::endl;

    return EXIT_SUCCESS;
}
