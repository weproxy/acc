//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <sstream>
#include <type_traits>

#include "co/god.h"

namespace gx {
namespace xx {
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// nil_t ...
struct nil_t {};
template <typename>
nil_t test(...);

// is_same ...
template <typename T, typename... X>
struct is_same {
    static constexpr bool value = god::xx::is_same<god::remove_cvref_t<T>, X...>::value;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// out ...
inline void out(std::ostream&) {}

// out ...
inline void out(std::ostream& s, int c) {
    char b[32];
    ::sprintf(b, "%d", c);
    s << b;
}

// out ...
template <typename T, typename... X>
void out(std::ostream& s, T&& t, X&&... x) {
    s << std::forward<T>(t) << " ";
    out(s, std::forward<X>(x)...);
}

// out ...
template <typename... T>
void out(std::ostream& s, T&&... t) {
    out(s, std::forward<T>(t)...);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// stringer ...
template <typename T>
struct detect_stringer {
   private:
    template <typename>
    static void test_reffn(...);

    template <typename>
    static void test_refop(...);

    template <typename>
    static void test_ptrfn(...);

    template <typename>
    static void test_ptrop(...);

    template <typename U>
    static auto test_reffn(U*) -> decltype(std::declval<U>().String());

    template <typename U>
    static auto test_refop(U*) -> decltype(std::declval<U>().operator string());

    template <typename U>
    static auto test_ptrfn(U) -> decltype(std::declval<U>()->String());

    template <typename U>
    static auto test_ptrop(U) -> decltype(std::declval<U>()->operator string());

    template <typename X>
    struct is_string {
        static constexpr bool value = god::xx::is_same<god::remove_cvref_t<X>, string, const char*, char*>::value;
    };

   public:
    // if has .String()
    static constexpr bool has_reffn = !std::is_pointer<T>::value && is_string<decltype(test_reffn<T>(nullptr))>::value;

    // if has .operator string()
    static constexpr bool has_refop = !std::is_pointer<T>::value && is_string<decltype(test_refop<T>(nullptr))>::value;

    // if has ->String()
    static constexpr bool has_ptrfn =
        std::is_pointer<T>::value && is_string<decltype(test_ptrfn<T>(std::declval<T>()))>::value;

    // if has ->operator string()
    static constexpr bool has_ptrop =
        std::is_pointer<T>::value && is_string<decltype(test_ptrop<T>(std::declval<T>()))>::value;
};

// is_stringer ...
// if has .String()
// if has .operator string()
// if has ->String()
// if has ->operator string()
template <typename T>
struct is_stringer {
    static constexpr bool value = (detect_stringer<T>::has_reffn || detect_stringer<T>::has_ptrfn ||
                                   detect_stringer<T>::has_refop || detect_stringer<T>::has_ptrop);
};

// to_string ...
// template <typename T>
// nil_t to_string(T t) {
//     return {};
// }

// to_string if has .String()
template <typename T, typename std::enable_if<detect_stringer<T>::has_reffn, int>::type = 0>
auto to_string(T t) -> decltype(t.String()) {
    return t.String();
}

// to_string if has .operator string()
template <typename T,
          typename std::enable_if<!detect_stringer<T>::has_reffn && detect_stringer<T>::has_refop, int>::type = 0>
auto to_string(T t) -> decltype(t.operator string()) {
    return t.operator string();
}

// to_string if has ->String()
template <typename T, typename std::enable_if<detect_stringer<T>::has_ptrfn, int>::type = 0>
auto to_string(T t) -> decltype(t->String()) {
    return t ? t->String() : "<nil>";
}

// to_string if has ->operator string()
template <typename T,
          typename std::enable_if<!detect_stringer<T>::has_ptrfn && detect_stringer<T>::has_ptrop, int>::type = 0>
auto to_string(T t) -> decltype(t->operator string()) {
    return t ? t->operator string() : "<nil>";
}

}  // namespace xx
}  // namespace gx
