#include "common/statistics.hpp"
#include "common/options.hpp"

#include <iostream>

namespace charge::common
{

Statistics::Statistics() noexcept
    : tail(Options::get().tail_statistics)
{
}

Statistics& Statistics::get() {
    thread_local Statistics instance;

#ifdef CHARGE_ENABLE_STATISTICS
    thread_local std::size_t access_counter = 0;
    if (instance.enabled && instance.tail)
    {
        access_counter++;
        if (access_counter > 10000)
        {
            std::cerr << instance.summary() << std::endl;
            access_counter = 0;
        }
    }
#endif


    return instance;
}

}
