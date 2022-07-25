#ifndef LIL_NARROW_CAST_H
#define LIL_NARROW_CAST_H
/*
 * Copyright (C) 2022 Earl Johnson
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Earl Johnson https://github.com/earl-sudo/lilcxx 2022
 */

#include "comp_info.h"

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
#if defined(HAVE_PRETTY_FUNCTION_MACRO)
        std::cerr << "EXCEPTION: narrowing_error (" << filename << ":" << id
                  << ") (TYPE=" << __PRETTY_FUNCTION__ << ")\n";
#else
        std::cerr << "EXCEPTION: narrowing_error (" << filename << ":" << id
                  << ") (TYPE=" << __FUNCTION__ << ")\n";
#endif
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
