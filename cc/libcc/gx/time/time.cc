//
// weproxy@foxmail.com 2022/10/03
//

#include "co/time.h"

#include "time.h"

namespace gx {
namespace time {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Before ...
bool Time::Before(const Time& t) const { return UnixMicro() < t.UnixMicro(); }

// Equal ...
bool Time::Equal(const Time& t) const { return t_.tv_sec == t.t_.tv_sec && t_.tv_nsec == t.t_.tv_nsec; }

// Add ...
Time Time::Add(const Duration& d) const {
    Time t(*this);
    t.t_.tv_sec += d.Seconds();
    return t;
}

// Sub ...
Duration Time::Sub(const Time& t) const { return Duration(this->UnixNano() - t.UnixNano()); }

// String ...
string Time::String() const { return ""; }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Now ...
Time Now() {
    // struct timeval tv;
    // ::gettimeofday(&tv, 0);
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return Time(t);
}

// AfterFunc ...
bool AfterFunc(const Time& t, const func<void()>& fn) { return false; }

// Sleep
void Sleep(const Duration& d) { sleep::ms(d.Milliseconds()); }

// Since ...
Duration Since(const Time& t) { return Now().Sub(t); }

}  // namespace time
}  // namespace gx
