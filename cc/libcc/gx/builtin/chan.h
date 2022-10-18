//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>

#include "r.h"
#include "def.h"
#include "go.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//
namespace gx {

namespace xx {
// chan ...
template <typename T>
struct chan {
    std::queue<T> q_;
    size_t N_;
    std::mutex mu_;
    std::condition_variable cv_;
    std::atomic<bool> closed_;

   public:
    explicit chan(size_t N = 1) : N_(N), closed_(false) {}
    ~chan() { Close(); }

    // Close ...
    error Close() {
        if (!closed_) {
            closed_ = true;
            cv_.notify_all();
        }
        return nil;
    }

    // << ...
    error operator<<(const T& t) {
        std::unique_lock<std::mutex> l(mu_);

        if (N_ > 0 && !closed_) {
            cv_.wait(l, [&] { return q_.size() < N_ || closed_; });
        }

        if (closed_) {
            return strerr_t::New("closed");
        }

        q_.emplace(t);

        cv_.notify_all();

        return nil;
    }

    // >> ...
    error operator>>(T& t) {
        std::unique_lock<std::mutex> l(mu_);

        if (!closed_) {
            cv_.wait(l, [&] { return q_.size() > 0 || closed_; });
        }

        if (closed_) {
            return strerr_t::New("closed");
        }

        t = q_.front();
        q_.pop();

        cv_.notify_all();

        return nil;
    }
};

}  // namespace xx

// chan ...
template <typename T>
struct chan {
    using D = xx::chan<T>;
    using P = Ref<D>;
    P p_;

    explicit chan(size_t N = 1) : p_(MakeRef<D>(N)) {}
    ~chan() { Close(); }

    // <<, >>
    error operator<<(const T& t) const { return (*p_) << t; }
    error operator>>(T& t) const { return (*p_) >> t; }

    // ()
    R<T, error> operator()() const {
        T t;
        auto e = this->operator>>(t);
        return {t, e};
    }

    // bool() ...
    operator bool() const { return !!p_; }

    // Close
    error Close() { return p_->Close(); }
};

// makechan ...
template <typename T>
chan<T> makechan(size_t N = 1) {
    return chan<T>(N);
}

}  // namespace gx

////////////////////////////////////////////////////////////////////////////////////////////////////
//
namespace gx {
namespace unitest {
void test_chan();
}  // namespace unitest
}  // namespace gx
