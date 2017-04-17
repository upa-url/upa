#ifndef INT_CAST_H
#define INT_CAST_H

#include <limits>
#include <stdexcept>
#include <type_traits>

namespace whatwg {

// utility class to get unsigned (abs) max, min values of (signed) integer type
template <typename T, typename UT = std::make_unsigned<T>::type>
struct unsigned_limit {
    static UT max()  {
        return static_cast<UT>(std::numeric_limits<T>::max());
    }
    static UT min()  {
        // http://en.cppreference.com/w/cpp/language/implicit_conversion
        // Integral conversions: If the destination type is unsigned, the resulting
        // value is the smallest unsigned value equal to the source value modulo 2n
        // where n is the number of bits used to represent the destination type.
        // https://en.wikipedia.org/wiki/Modular_arithmetic#Congruence_relation
        return static_cast<UT>(0) - static_cast<UT>(std::numeric_limits<T>::min());
    }
};

template <typename Out, typename T,
    typename UT = std::make_unsigned<T>::type,
    typename = typename std::enable_if<std::is_integral<T>::value, Out>::type>
inline Out checked_diff(T a, T b) {
    if (a >= b) {
        const UT diff = static_cast<UT>(a) - static_cast<UT>(b);
        if (diff <= unsigned_limit<Out>::max())
            return static_cast<Out>(diff);
    } else if (std::is_signed<Out>::value) {
        // b > a ==> diff >= 1
        const UT diff = static_cast<UT>(b) - static_cast<UT>(a);
        if (diff <= unsigned_limit<Out>::min())
            return static_cast<Out>(0) - static_cast<Out>(diff - 1) - 1;
    }
    throw std::length_error("too big difference");
}


}

#endif // INT_CAST_H
