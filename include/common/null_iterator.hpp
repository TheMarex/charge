#ifndef CHARGE_COMMON_NULL_ITERATOR_HPP
#define CHARGE_COMMON_NULL_ITERATOR_HPP

#include <iterator>

namespace charge::common {

template <typename T>
class NullOutputIter : public std::iterator<std::output_iterator_tag, void, void, void, void> {
  public:
    NullOutputIter &operator=(const T &) { return *this; }
    NullOutputIter &operator*() { return *this; }
    NullOutputIter &operator++() { return *this; }
    NullOutputIter operator++(int) { return *this; }
};
}

#endif
