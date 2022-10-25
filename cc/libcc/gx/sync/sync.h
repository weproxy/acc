//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <mutex>

#include "gx/gx.h"

namespace gx {
namespace sync {

// WaitGroup ...
// using WaitGroup = co::WaitGroup;
class WaitGroup {
   public:
    // initialize the counter as @n
    explicit WaitGroup(uint32 n) : wg_(n){};

    // the counter is 0 by default
    WaitGroup() : wg_(0) {}

    WaitGroup(WaitGroup&& wg) : wg_(wg.wg_) {}

    // copy constructor, allow WaitGroup to be captured by value in lambda.
    WaitGroup(const WaitGroup& wg) : wg_(wg.wg_) {}

    void operator=(const WaitGroup&) = delete;

    // increase WaitGroup counter by n (1 by default)
    void Add(uint32 n = 1) const { wg_.add(n); }

    // decrease WaitGroup counter by 1
    void Done() const { wg_.done(); }

    // blocks until the counter becomes 0
    void Wait() const { wg_.wait(); }

   private:
    co::WaitGroup wg_;
};

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
