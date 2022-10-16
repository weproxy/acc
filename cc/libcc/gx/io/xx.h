//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <type_traits>

#include "gx/builtin/builtin.h"

namespace gx {
namespace io {
namespace xx {

// has_read ...
// ----> R<int, error> Read(void*, size_t)
template <typename T>
struct has_read {
   private:
    template <typename U>
    static auto test(U) -> decltype(std::declval<U>()->Read(slice<byte>{}));

   public:
    static constexpr bool value = gx::xx::is_same<decltype(test<T>(std::declval<T>())), R<int, error>>::value;
};

// has_write ...
// ----> R<int, error> Write(const void*, size_t)
template <typename T>
struct has_write {
   private:
    template <typename U>
    static auto test(U) -> decltype(std::declval<U>()->Write(slice<byte>{}));

   public:
    static constexpr bool value = gx::xx::is_same<decltype(test<T>(std::declval<T>())), R<int, error>>::value;
};

// has_close ...
// ----> void Close();
template <typename T>
struct has_close {
   private:
    template <typename U>
    static auto test(U) -> decltype(std::declval<U>()->Close());

   public:
    static constexpr bool value = !gx::xx::is_same<decltype(test<T>(std::declval<T>())), gx::xx::nil_t>::value;
};

// has_read_write ...
template <typename T>
struct has_read_write {
    static constexpr bool value = has_read<T>::value && has_write<T>::value;
};

// has_read_close ...
template <typename T>
struct has_read_close {
    static constexpr bool value = has_read<T>::value && has_close<T>::value;
};

// has_write_close ...
template <typename T>
struct has_write_close {
    static constexpr bool value = has_write<T>::value && has_close<T>::value;
};

// has_read_write_close ...
template <typename T>
struct has_read_write_close {
    static constexpr bool value = has_read_write<T>::value && has_close<T>::value;
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
template <typename T, typename std::enable_if<xx::has_close<T>::value, int>::type = 0>
void call_close_read(T t) {
    t->Close();
}

// call_close_write ...
template <typename T, typename std::enable_if<xx::has_close_write<T>::value, int>::type = 0>
void call_close_write(T t) {
    t->CloseWrite();
}

// call_close_write ...
template <typename T, typename std::enable_if<xx::has_close<T>::value, int>::type = 0>
void call_close_write(T t) {
    t->Close();
}

}  // namespace xx
}  // namespace io
}  // namespace gx
