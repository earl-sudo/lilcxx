#ifndef LIL_NARROW_CAST_H
#define LIL_NARROW_CAST_H

#include <cassert>
#include <exception>
#include <iostream>
#include <type_traits>

namespace details {
// FROM: https://codereview.stackexchange.com/questions/230931/an-explicit-cast-for-narrowing-numeric-conversions
    template <typename T, typename U>
    constexpr bool is_same_signedness = std::is_signed<T>::value ==
                                        std::is_signed<U>::value;

    template <typename T, typename U>
    constexpr bool can_fully_represent =
                           std::is_same<T, U>::value ||
                           ( std::is_integral<T>::value && std::is_integral<U>::value &&
                             ( ( std::is_signed<T>::value && sizeof(T) >  sizeof(U) ) ||
                               ( is_same_signedness<T, U> && sizeof(T) >= sizeof(U) ) ) ) ||
                           ( std::is_floating_point<T>::value &&
                             std::is_floating_point<U>::value && sizeof(T) >= sizeof(U) );

    template <typename T, typename U>
    constexpr bool static_cast_changes_value(U u) noexcept {
        const auto t = static_cast<T>(u);
        // this should catch most cases, but may miss dodgy unsigned to signed conversion or vice-versa
        if (static_cast<U>(t) != u) return true;
        if (std::is_signed<T>::value != std::is_signed<U>::value && ((t <
                                                                      T{}) != (u < U{}))) return true;
        return false;
    }
} // namespace details

// #TODO: unchecked cast for types where some loss of precision (and therefore assertion failure) is expected?
template <typename T, typename U>
constexpr T narrow_cast(U&& u) noexcept {
    static_assert(!details::can_fully_represent<T, U>, "we shouldn't be using narrow_cast for casts that aren't actually narrowing");
            assert(!details::static_cast_changes_value<T>(u));
    return static_cast<T>(std::forward<U>(u));
}

struct narrowing_error : public std::bad_cast {
    const char* filename_ = nullptr;
    uint32_t line_ = 0;
    narrowing_error(const char* filenameD, uint32_t idD) : line_(idD), filename_(filenameD)  { }
};

template <typename CASTTO_TYPE, typename CASTFROM_TYPE>
constexpr CASTTO_TYPE narrow_cast_checked(CASTFROM_TYPE u, const char* filename = nullptr, uint32_t id = 0) {
    static_assert(!details::can_fully_represent<CASTTO_TYPE, CASTFROM_TYPE>,
                  "we shouldn't be using narrow_cast for casts that aren't actually narrowing");
    if (details::static_cast_changes_value<CASTTO_TYPE>(u)) {
        std::cerr << "EXCEPTION: narrowing_error (" << filename << ":" << id
                  << ") (TYPE=" << __PRETTY_FUNCTION__ << ")\n";
        throw narrowing_error(filename, id);
    }

    return static_cast<CASTTO_TYPE>(u);
}

#include <cstdint>

// If the runtime type conversion would lose information this will throw an narrowing_error exception.
#define NARROW(TOVAR,FROMVAR, ID) narrow_cast_checked<decltype( TOVAR ), decltype( FROMVAR )>( FROMVAR , ID )
#define NARROWT(TOVARTYPE,FROMVAR, ID) narrow_cast_checked< TOVARTYPE , decltype( FROMVAR )>( FROMVAR , ID )
#define _NT(TOVARTYPE,FROMVAR) narrow_cast_checked< TOVARTYPE , decltype( FROMVAR )>( FROMVAR , __FILE__, __LINE__ )

#endif //LIL_NARROW_CAST_H
