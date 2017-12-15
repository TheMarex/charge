#include "common/signal_handler.hpp"

#include <csignal>
#include <iostream>

namespace charge::common
{
    SignalHandler::SignalHandler()
    {
        std::signal(SIGINT, SignalHandler::handle);
    }

    SignalHandler& SignalHandler::get()
    {
        static SignalHandler handler;
        return handler;
    }

    void SignalHandler::set(int signal, std::function<void()> fn)
    {
        if (signal != SIGINT)
            throw std::runtime_error("Can only register for SIGINT");

        std::lock_guard gurard {mutex};

        callbacks[signal].push_back(fn);
    }

    void SignalHandler::handle(int signal)
    {
        auto& instance = SignalHandler::get();
        std::lock_guard gurard {instance.mutex};

        auto iter = instance.callbacks.find(signal);
        if(iter == instance.callbacks.end())
            return;

        for (const auto &cb : iter->second)
            cb();

        std::signal(signal, SIG_DFL);
    }
}
