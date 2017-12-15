#ifndef CHARGE_TIMED_LOGGER_HPP
#define CHARGE_TIMED_LOGGER_HPP

#include <chrono>
#include <iostream>
#include <string>

namespace charge {
namespace common {
struct TimedLogger {
    TimedLogger(const std::string &msg)
        : start(std::chrono::high_resolution_clock::now()) {
        std::cerr << msg << "... " << std::flush;
    }

    void finished() {
        auto end = std::chrono::high_resolution_clock::now();
        auto diff = end - start;
        std::cerr << std::chrono::duration_cast<std::chrono::microseconds>(diff).count()/1000. << " ms."
                  << std::endl;
    }

    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};
}
}

#endif
