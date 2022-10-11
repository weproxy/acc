//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <type_traits>

#include "builtin/builtin.h"

namespace gx {
namespace io {
namespace xx {

// is_reader ...
// ----> R<size_t, error> Read(void*, size_t)
template <typename T>
struct is_reader {
   private:
    template <typename U>
    static auto test(U) -> decltype(std::declval<U>()->Read((void*)0, (size_t)0));

   public:
    static constexpr bool value = gx::xx::is_same<decltype(test<T>(std::declval<T>())), R<size_t, error>>::value;
};

// is_writer ...
// ----> R<size_t, error> Write(const void*, size_t)
template <typename T>
struct is_writer {
   private:
    template <typename U>
    static auto test(U) -> decltype(std::declval<U>()->Write((const void*)0, (size_t)0));

   public:
    static constexpr bool value = gx::xx::is_same<decltype(test<T>(std::declval<T>())), R<size_t, error>>::value;
};

// is_closer ...
// ----> void Close();
template <typename T>
struct is_closer {
   private:
    template <typename U>
    static auto test(U) -> decltype(std::declval<U>()->Close());

   public:
    static constexpr bool value = !gx::xx::is_same<decltype(test<T>(std::declval<T>())), gx::xx::nil_t>::value;
};

// is_read_writer ...
template <typename T>
struct is_read_writer {
    static constexpr bool value = is_reader<T>::value && is_writer<T>::value;
};

// is_read_closer ...
template <typename T>
struct is_read_closer {
    static constexpr bool value = is_reader<T>::value && is_closer<T>::value;
};

// is_write_closer ...
template <typename T>
struct is_write_closer {
    static constexpr bool value = is_writer<T>::value && is_closer<T>::value;
};

// is_read_write_closer ...
template <typename T>
struct is_read_write_closer {
    static constexpr bool value = is_read_writer<T>::value && is_closer<T>::value;
};

}  // namespace xx
}  // namespace io
}  // namespace gx
