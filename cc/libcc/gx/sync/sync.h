//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <mutex>

namespace gx {
namespace sync {

///////////////////////////////////////////////////////////////////////////////
//
// mutex_t ...
template <typename T>
class mutex_t {
   public:
    using locker_t = std::unique_lock<T>;

    // Lock ...
    virtual void Lock() { mu_.lock(); }

    // Unlock ...
    virtual void Unlock() { mu_.unlock(); }

    // Mutex ...
    T& Mutex() { return mu_; }

    // operator ...
    operator T&() { return mu_; }
    operator const T&() const { return mu_; }

   protected:
    T mu_;
};

using Mutex = mutex_t<std::mutex>;
using RecursiveMutex = mutex_t<std::recursive_mutex>;
using MutexLocker = Mutex::locker_t;
using RecursiveMutexLocker = RecursiveMutex::locker_t;

///////////////////////////////////////////////////////////////////////////////
//
// Once ...
struct Once {
    // Do ...
    template <class Fn, class... Args>
    void Do(Fn&& fn, Args&&... args) {
        std::call_once(flag_, fn, args...);
    }

   private:
    std::once_flag flag_;
};

}  // namespace sync
}  // namespace gx
