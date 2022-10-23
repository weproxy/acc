//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <sys/time.h>

#include "gx/gx.h"

namespace gx {
namespace time {

const int64 minDuration = uint64(-1) << 63;
const int64 maxDuration = (uint64(1) << 63) - 1;

//////////////////////////////////////////////////////////////////////////////////
// A Duration represents the elapsed time between two instants
// as an int64 nanosecond count. The representation limits the
// largest representable duration to approximately 290 years.
struct Duration {
    Duration(int64 d = 0) : d_(d) {}

    // operator int64() ...
    operator int64() const { return d_; }

    // Hours returns the duration as a floating point number of hours.
    double Hours() const;
    // Minutes returns the duration as a floating point number of minutes.
    double Minutes() const;
    // Seconds returns the duration as a floating point number of seconds.
    double Seconds() const;

    // Milliseconds returns the duration as an integer millisecond count.
    int64 Milliseconds() const { return d_ / int64(1e6); }
    // Microseconds returns the duration as an integer microsecond count.
    int64 Microseconds() const { return d_ / 1e3; }
    // Nanoseconds returns the duration as an integer nanosecond count.
    int64 Nanoseconds() const { return d_; }

    // String returns a string representing the duration in the form "72h3m0.5s".
    // Leading zero units are omitted. As a special case, durations less than one
    // second format use a smaller unit (milli-, micro-, or nanoseconds) to ensure
    // that the leading digit is non-zero. The zero duration formats as 0s.
    string String() const;

    // Truncate returns the result of rounding d toward zero to a multiple of m.
    // If m <= 0, Truncate returns d unchanged.
    Duration Truncate(Duration m) {
        if (m <= 0) {
            return *this;
        }
        return Duration(d_ - d_ % m.d_);
    }

    // Round returns the result of rounding d to the nearest multiple of m.
    // The rounding behavior for halfway values is to round away from zero.
    // If the result exceeds the maximum (or minimum)
    // value that can be stored in a Duration,
    // Round returns the maximum (or minimum) duration.
    // If m <= 0, Round returns d unchanged.
    Duration Round(Duration m) {
        // TODO...
        return m;
    }

    // Abs returns the absolute value of d.
    // As a special case, math.MinInt64 is converted to math.MaxInt64.
    Duration Abs() {
        if (d_ >= 0) {
            return *this;
        } else if (d_ == minDuration) {
            return maxDuration;
        }
        return -d_;
    }

   private:
    int64 d_{0};  // Nanoseconds
};

////////////////////////////////////////////////////////////////////////////////
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

    // After reports whether the time instant t is after u.
    bool After(const Time& u) const;

    // Before reports whether the time instant t is before u.
    bool Before(const Time& u) const;

    // Equal ...
    bool Equal(const Time& t) const;

    // Add returns the time t+d.
    Time Add(const Duration& d) const;

    // AddDate returns the time corresponding to adding the
    // given number of years, months, and days to t.
    // For example, AddDate(-1, 2, 3) applied to January 1, 2011
    // returns March 4, 2010.
    //
    // AddDate normalizes its result in the same way that Date does,
    // so, for example, adding one month to October 31 yields
    // December 1, the normalized form for November 31.
    Time AddDate(int years, int months, int days);

    // Sub returns the duration t-u. If the result exceeds the maximum (or minimum)
    // value that can be stored in a Duration, the maximum (or minimum) duration
    // will be returned.
    // To compute t-d for a duration d, use t.Add(-d).
    Duration Sub(const Time& t) const;

    // Unix returns t as a Unix time, the number of seconds elapsed
    // since January 1, 1970 UTC. The result does not depend on the
    // location associated with t.
    // Unix-like operating systems often record time as a 32-bit
    // count of seconds, but since the method here returns a 64-bit
    // value it is valid for billions of years into the past or future.
    int64 Unix() const { return t_.tv_sec + t_.tv_nsec / 1e9; }

    // UnixMilli returns t as a Unix time, the number of milliseconds elapsed since
    // January 1, 1970 UTC. The result is undefined if the Unix time in
    // milliseconds cannot be represented by an int64 (a date more than 292 million
    // years before or after 1970). The result does not depend on the
    // location associated with t.
    int64 UnixMilli() const { return t_.tv_sec * 1e3 + t_.tv_nsec / 1e6; }

    // UnixMicro returns t as a Unix time, the number of microseconds elapsed since
    // January 1, 1970 UTC. The result is undefined if the Unix time in
    // microseconds cannot be represented by an int64 (a date before year -290307 or
    // after year 294246). The result does not depend on the location associated
    // with t.
    int64 UnixMicro() const { return t_.tv_sec * 1e6 + t_.tv_nsec / 1e3; }

    // UnixNano returns t as a Unix time, the number of nanoseconds elapsed
    // since January 1, 1970 UTC. The result is undefined if the Unix time
    // in nanoseconds cannot be represented by an int64 (a date before the year
    // 1678 or after 2262). Note that this means the result of calling UnixNano
    // on the zero Time is undefined. The result does not depend on the
    // location associated with t.
    int64 UnixNano() const { return t_.tv_sec * 1e9 + t_.tv_nsec; }

   public:
    // operator bool() ...
    operator bool() const { return !IsZero(); }

    // String ...
    string String() const;

    // Format ...
    string Format(const string& fmt) const;

    //    private:
    struct timespec t_;
};

////////////////////////////////////////////////////////////////////////////////
// Common durations. There is no definition for units of Day or larger
// to avoid confusion across daylight savings time zone transitions.
//
// To count the number of units in a Duration, divide:
//
//	second := time.Second
//	fmt.Print(int64(second/time.Millisecond)) // prints 1000
//
// To convert an integer number of units to a Duration, multiply:
//
//	seconds := 10
//	fmt.Print(time.Duration(seconds)*time.Second) // prints 10s
extern const Duration Nanosecond;
extern const Duration Microsecond;
extern const Duration Millisecond;
extern const Duration Second;
extern const Duration Minute;
extern const Duration Hour;

////////////////////////////////////////////////////////////////////////////////
//

// Now ...
Time Now();

// IsZero ...
inline bool IsZero(const Time& t) { return t.IsZero(); }

// AfterFunc waits for the duration to elapse and then calls f
// in its own goroutine. It returns a Timer that can
// be used to cancel the call using its Stop method.
bool AfterFunc(const Time& t, const func<void()>& fn);

// Sleep
void Sleep(const Duration& d);

// Since returns the time elapsed since t.
// It is shorthand for time.Now().Sub(t).
Duration Since(const Time& t);

// Until returns the duration until t.
// It is shorthand for t.Sub(time.Now()).
Duration Until(const Time& t);

}  // namespace time
}  // namespace gx
