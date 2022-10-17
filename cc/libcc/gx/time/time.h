//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <sys/time.h>

#include "gx/gx.h"

namespace gx {
namespace time {

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Duration ...
struct Duration {
    Duration(int64 d = 0) : d_(d) {}

    // operator int64()
    operator int64() const { return d_; }

    double Hours() const;
    double Minutes() const;
    double Seconds() const;

    int64 Milliseconds() const { return d_ / int64(1e6); }
    int64 Microseconds() const { return d_ / 1e3; }
    int64 Nanoseconds() const { return d_; }

    // String ...
    string String() const;

   private:
    int64 d_{0};  // Nanoseconds
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Time ...
struct Time {
    Time() {
        t_.tv_sec = 0;
        t_.tv_nsec = 0;
    }
    Time(struct timespec& t) { t_ = t; }
    Time(const Time& t) { t_ = t.t_; }

    // IsZero ...
    bool IsZero() const { return t_.tv_sec <= 0 && t_.tv_nsec <= 0; }

    // Before ...
    bool Before(const Time& t) const;

    // Equal ...
    bool Equal(const Time& t) const;

    // Add ...
    Time Add(const Duration& d) const;

    // Sub ...
    Duration Sub(const Time& t) const;

    // bool operator
    operator bool() const { return !IsZero(); }

    // String ...
    string String() const;

    // Format ...
    string Format(const string& fmt) const;

    int64 Unix() const { return t_.tv_sec + t_.tv_nsec / 1e9; }
    int64 UnixMilli() const { return t_.tv_sec * 1e3 + t_.tv_nsec / 1e6; }
    int64 UnixMicro() const { return t_.tv_sec * 1e6 + t_.tv_nsec / 1e3; }
    int64 UnixNano() const { return t_.tv_sec * 1e9 + t_.tv_nsec; }

//    private:
    struct timespec t_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// const ...
extern const Duration Nanosecond;
extern const Duration Microsecond;
extern const Duration Millisecond;
extern const Duration Second;
extern const Duration Minute;
extern const Duration Hour;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

// Now ...
Time Now();

// IsZero ...
inline bool IsZero(const Time& t) { return t.IsZero(); }

// AfterFunc ...
bool AfterFunc(const Time& t, const std::function<void()>& fn);

// Sleep
void Sleep(const Duration& d);

// Since ...
Duration Since(const Time& t);

}  // namespace time
}  // namespace gx
