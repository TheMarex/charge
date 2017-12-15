#ifndef CHARGE_COMMON_PARALLEL_FOR_HPP
#define CHARGE_COMMON_PARALLEL_FOR_HPP

#include "common/irange.hpp"

#include <cmath>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace charge::common {
template <typename IndexT, typename FunctionT>
void parallel_for(range<IndexT> full_range, FunctionT fn, std::size_t num_threads) {
    std::deque<range<IndexT>> queue;
    IndexT max_grain_size = std::ceil(full_range.size() / (double)num_threads);
    IndexT min_grain_size = 10;
    IndexT sub_range_size = std::max(min_grain_size, max_grain_size / 10);

    for (IndexT begin = 0; begin < full_range.size(); begin += sub_range_size) {
        queue.push_back(irange<IndexT>(begin, std::min(full_range.size(), begin + sub_range_size)));
    }

    std::mutex queue_mutex;
    std::vector<std::thread> threads;

    const auto get_work = [&queue_mutex, &queue]() {
        std::lock_guard<std::mutex> guard{queue_mutex};

        range<IndexT> work{0, 0};

        if (queue.size() > 0) {
            work = queue.front();
            queue.pop_front();
        }
        return work;
    };

    for (auto idx : irange<std::size_t>(0, num_threads)) {
        (void) idx;
        threads.push_back(std::thread([fn, &get_work]() {
            range<IndexT> work = get_work();

            while (work.size() > 0) {
                fn(work);
                work = get_work();
            }
        }));
    }

    for (auto &thread : threads)
        thread.join();
}
}

#endif
