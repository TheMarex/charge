#ifndef CHARGE_COMMON_DONT_OPTIMIZE_HPP
#define CHARGE_COMMON_DONT_OPTIMIZE_HPP

namespace charge {
namespace common {

#ifdef _WIN32
#pragma optimize("", off)
template <class T>
void dont_optimize_away(T &&datum) {
    T local = datum;
}
#pragma optimize("", on)
#elif __GNUC__
#ifdef __clang__
template <class T>
void __attribute__((optnone))  dont_optimize_away(T &&datum) {
    T local = datum;
}
#else
template <class T>
void __attribute__((optimize("O0")))  dont_optimize_away(T &&datum) {
    T local = datum;
}
#endif
#endif
}
}

#endif
