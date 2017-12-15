#include <iostream>

#include "server/http.hpp"

int main(int argc, const char *argv[]) {
    if (argc < 3) {
        std::cout << argv[0] << " base_path capacity" << std::endl;
        return EXIT_FAILURE;
    }

    const std::string base_path = argv[1];
    const double capacity = std::stof(argv[2]);

    const charge::server::Charge charge(base_path, capacity);
    charge::server::HTTPServer server(charge, 5000);

    std::cerr << "Listening on port 5000..." << std::endl;
    server.wait();

    return EXIT_SUCCESS;
}
