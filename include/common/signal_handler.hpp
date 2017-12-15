#ifndef COMMON_SIGNAL_HANLDER_HPP
#define COMMON_SIGNAL_HANLDER_HPP

#include <functional>
#include <mutex>

namespace charge::common {
class SignalHandler {
  public:
    void set(int signal, std::function<void()> fn);
    static SignalHandler &get();

  private:
    SignalHandler();
    static void handle(int signal);

    std::mutex mutex;
    std::unordered_map<int, std::vector<std::function<void()>>> callbacks;
};
} // namespace charge::common

#endif
