//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <type_traits>

#include "gx/builtin/builtin.h"

namespace gx {
namespace io {
namespace xx {

// is_reader ...
// ----> R<int, error> Read(void*, size_t)
template <typename T>
struct is_reader {
   private:
    template <typename U>
    static auto test(U) -> decltype(std::declval<U>()->Read(slice<byte>{}));

   public:
    static constexpr bool value = gx::xx::is_same<decltype(test<T>(std::declval<T>())), R<int, error>>::value;
};

// is_writer ...
// ----> R<int, error> Write(const void*, size_t)
template <typename T>
struct is_writer {
   private:
    template <typename U>
    static auto test(U) -> decltype(std::declval<U>()->Write(slice<byte>{}));

   public:
    static constexpr bool value = gx::xx::is_same<decltype(test<T>(std::declval<T>())), R<int, error>>::value;
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// has_close_read ...
// ----> void CloseRead();
template <typename T>
struct has_close_read {
   private:
    template <typename U>
    static auto test(U) -> decltype(std::declval<U>()->CloseRead());

   public:
    static constexpr bool value = !gx::xx::is_same<decltype(test<T>(std::declval<T>())), gx::xx::nil_t>::value;
};

// has_close_write ...
// ----> void CloseWrite();
template <typename T>
struct has_close_write {
   private:
    template <typename U>
    static auto test(U) -> decltype(std::declval<U>()->CloseWrite());

   public:
    static constexpr bool value = !gx::xx::is_same<decltype(test<T>(std::declval<T>())), gx::xx::nil_t>::value;
};

// call_close_read ...
template <typename T, typename std::enable_if<xx::has_close_read<T>::value, int>::type = 0>
void call_close_read(T t) {
    t->CloseRead();
}

// call_close_read ...
template <typename T, typename std::enable_if<xx::is_closer<T>::value, int>::type = 0>
void call_close_read(T t) {
    t->Close();
}

// call_close_write ...
template <typename T, typename std::enable_if<xx::has_close_write<T>::value, int>::type = 0>
void call_close_write(T t) {
    t->CloseWrite();
}

// call_close_write ...
template <typename T, typename std::enable_if<xx::is_closer<T>::value, int>::type = 0>
void call_close_write(T t) {
    t->Close();
}

}  // namespace xx
}  // namespace io
}  // namespace gx
