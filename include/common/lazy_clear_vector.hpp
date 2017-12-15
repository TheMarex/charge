#ifndef WEIGHT_CONTAINER_HPP
#define WEIGHT_CONTAINER_HPP

#include <cstdint>
#include <vector>
#include <limits>

namespace charge
{
namespace common
{

template <typename ElementT> class LazyClearVector {
  public:
	using value_t = ElementT;
	using counter_t = std::uint8_t;
    static const constexpr counter_t INVALID_GENERATION = std::numeric_limits<counter_t>::max();

	LazyClearVector(std::size_t size_, value_t default_value_)
		: default_value(std::move(default_value_)), generations(size_, 0),
		  elements(size_, default_value), generation_counter(0) {}

	void clear() {
        ++generation_counter;

        if (generation_counter == INVALID_GENERATION)
        {
            std::fill(generations.begin(), generations.end(), INVALID_GENERATION);
            generation_counter = 0;
        }
    }

	const value_t &peek(std::size_t index) const { return (*this)[index]; }

	const value_t &operator[](std::size_t index) const {
		if (generations[index] == generation_counter) {
			return elements[index];
		}
		return default_value;
	}

	value_t &operator[](std::size_t index) {
		if (generations[index] == generation_counter) {
			return elements[index];
		} else {
			elements[index] = default_value;
			generations[index] = generation_counter;
			return elements[index];
		}
	}

	std::size_t size() const {
		return elements.size();
	}

  private:
	value_t default_value;
	counter_t generation_counter;
	std::vector<counter_t> generations;
	std::vector<value_t> elements;
};

template<> class LazyClearVector<bool> {
  public:
	using value_t = bool;
	using counter_t = std::uint8_t;
    static const constexpr counter_t INVALID_GENERATION = std::numeric_limits<counter_t>::max();

	LazyClearVector(std::size_t size_, value_t default_value_)
		: default_value(std::move(default_value_)), generations(size_, 0),
		  elements(size_, default_value), generation_counter(0) {}

	void clear() {
        ++generation_counter;

        if (generation_counter == INVALID_GENERATION)
        {
            std::fill(generations.begin(), generations.end(), INVALID_GENERATION);
            generation_counter = 0;
        }
    }

	value_t peek(std::size_t index) const { return (*this)[index]; }

	value_t operator[](std::size_t index) const {
		if (generations[index] == generation_counter) {
			return elements[index];
		}
		return default_value;
	}

    std::vector<bool>::reference operator[](std::size_t index) {
		if (generations[index] == generation_counter) {
			return elements[index];
		} else {
			elements[index] = default_value;
			generations[index] = generation_counter;
			return elements[index];
		}
	}

	std::size_t size() const {
		return elements.size();
	}

  private:
	value_t default_value;
	counter_t generation_counter;
	std::vector<counter_t> generations;
	std::vector<value_t> elements;
};

}
}

#endif
