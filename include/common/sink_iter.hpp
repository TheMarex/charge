#ifndef CHARGE_COMMON_SINK_ITER_HPP
#define CHARGE_COMMON_SINK_ITER_HPP

#include <iterator>
#include <type_traits>

namespace charge::common {

template <typename FnT>
class SinkIter : public std::iterator<std::output_iterator_tag, void, void, void, void> {
public:
    SinkIter(SinkIter&&) = default;
    SinkIter(const SinkIter&) = default;
    SinkIter& operator=(SinkIter&&) = default;
    SinkIter& operator=(const SinkIter&) = default;

    SinkIter(FnT function_) : function{std::move(function_)} {}

    const SinkIter& operator++() const { return *this; }
    const SinkIter& operator++(int) const { return *this; }

    struct Proxy
    {
        Proxy() = default;
        Proxy(Proxy&&) = default;
        Proxy(const Proxy&) = default;
        Proxy& operator=(Proxy&&) = default;
        Proxy& operator=(const Proxy&) = default;

        template<typename T, typename = std::enable_if_t<!std::is_same_v<T, Proxy>>>
        auto operator=(const T& value) const
        {
            function(value);
            return *this;
        }

        template<typename T, typename = std::enable_if_t<!std::is_same_v<T, Proxy>>>
        auto operator=(T&& value) const
        {
            function(std::move(value));
            return *this;
        }

        const FnT &function;
    };

    Proxy operator*() const { return Proxy {function}; }

  private:
    FnT function;
};

template <typename TransformFn>
auto make_sink_iter(TransformFn transform)
{
    return SinkIter<TransformFn>{transform};
}

}

#endif
