#include "common/options.hpp"

#include <string>

namespace charge::common {
namespace {

bool is_on(const std::string &name) {
    const auto *env_ptr = std::getenv(name.c_str());
    if (env_ptr != nullptr) {
        std::string env(env_ptr);
        return env == "1" || env == "on" || env == "ON" || env == "On";
    }
    return false;
}
}

Options::Options() noexcept
    : tail_statistics(is_on("CHARGE_TAIL_STATISTICS")),
      tail_experiment(is_on("CHARGE_TAIL_EXPERIMENT")), tail_memory(is_on("CHARGE_TAIL_MEMORY")) {}

const Options &Options::get() {
    static Options options;

    return options;
}
}
