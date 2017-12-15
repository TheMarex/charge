#ifndef CHARGE_COMMON_ADAPTER_ITER_HPP
#define CHARGE_COMMON_ADAPTER_ITER_HPP

#include <iterator>
#include <type_traits>

namespace charge::common {

template <typename InIter, typename OutType, typename TransformFn>
class AdapterIter : public std::iterator<std::random_access_iterator_tag, OutType> {
public:
    AdapterIter() = default;
    AdapterIter(AdapterIter&&) = default;
    AdapterIter(const AdapterIter&) = default;
    AdapterIter& operator=(AdapterIter&&) = default;
    AdapterIter& operator=(const AdapterIter&) = default;

    AdapterIter(InIter in_, TransformFn transform_) : iter(std::move(in_)), transform(std::move(transform_)) {}

    AdapterIter& operator++() { iter++; return *this; }
    AdapterIter& operator--() { iter--; return *this; }
    AdapterIter& operator+=(int offset) { iter += offset; return *this; }
    AdapterIter& operator-=(int offset) { iter -= offset; return *this; }

    AdapterIter operator+(int offset) { return AdapterIter{iter + offset, transform}; }
    AdapterIter operator-(int offset) { return AdapterIter{iter - offset, transform}; }
    int operator+(const AdapterIter &other) { return iter + other.iter; }
    int operator-(const AdapterIter &other) { return iter - other.iter; }

    OutType& operator*() { return transform(*iter); }
    const OutType& operator*() const { return transform(*iter); }

    OutType& operator[](int offset) { return transform(iter[offset]); }
    const OutType& operator[](int offset) const { return transform(iter[offset]); }

    bool operator==(const AdapterIter& other) const { return iter == other.iter; }
    bool operator!=(const AdapterIter& other) const { return iter != other.iter; }
    bool operator<(const AdapterIter& other) const { return iter < other.iter; }
    bool operator>(const AdapterIter& other) const { return iter > other.iter; }

  private:
    TransformFn transform;
    InIter iter;
};

template <typename InIter, typename TransformFn>
auto make_adapter_iter(InIter iter, TransformFn transform)
{
    using OutType = std::remove_reference_t<decltype(transform(*iter))>;
    return AdapterIter<InIter, OutType, TransformFn>{iter, transform};
}

}

#endif
