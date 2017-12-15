#ifndef PROGRESS_BAR_HPP
#define PROGRESS_BAR_HPP

#include <iostream>
#include <mutex>
#include <atomic>

namespace charge::common
{

struct ProgressBar {
	ProgressBar(std::size_t num_steps) : num_step(0), to_permille(1000.0 / num_steps), last_permille(-1) {}

	~ProgressBar() { std::cerr << std::endl; }

	void update(std::size_t step_size=1) {
        num_step += step_size;
		int permille = to_permille * num_step;
		if (permille > last_permille) {
            std::lock_guard<std::mutex> guard(mutex);

			std::cerr << "\r[" << (permille / 10.) << "%]";
			for (int i = 0; i < (permille / 10.); ++i)
				std::cerr << "|";
			last_permille = permille;
			std::cerr << std::flush;
		}
	}

    std::mutex mutex;
    std::atomic_uint num_step;
	double to_permille;
	int last_permille;
};

}

#endif
