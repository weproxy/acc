//
// weproxy@foxmail.com 2022/10/03
//

#include "co/time.h"
#include "time.h"

namespace gx {
namespace time {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
const Duration Nanosecond(1);
const Duration Microsecond(1e3 * int64(Nanosecond));
const Duration Millisecond(1e3 * int64(Microsecond));
const Duration Second(1e3 * int64(Millisecond));
const Duration Minute(60 * int64(Second));
const Duration Hour(60 * int64(Minute));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
double Duration::Hours() const {
    int64 hour = d_ / int64(Hour);
    int64 nsec = d_ % int64(Hour);
    return double(hour) + double(nsec) / (double)(60.0f * 60.0f * 1e9);
}

double Duration::Minutes() const {
    int64 min = d_ / int64(Minute);
    int64 nsec = d_ % int64(Minute);
    return double(min) + double(nsec) / (double)(60.0f * 1e9);
}

double Duration::Seconds() const {
    int64 sec = d_ / int64(Second);
    int64 nsec = d_ % int64(Second);
    return double(sec) + double(nsec) / (double)1e9;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// fmtFrac formats the fraction of v/10**prec (e.g., ".12345") into the
// tail of buf, omitting trailing zeros. It omits the decimal
// point too when the fraction is 0. It returns the index where the
// output bytes begin and the value v/10**prec.
R<int, uint64> fmtFrac(byte buf[], int w, uint64 v, int prec) {
    // Omit trailing zeros up to and including decimal point.
    bool print = false;
    for (int i = 0; i < prec; i++) {
        uint64 digit = v % 10;
        print = print || digit != 0;
        if (print) {
            w--;
            buf[w] = byte(digit) + '0';
        }
        v /= 10;
    }
    if (print) {
        w--;
        buf[w] = '.';
    }
    return {w, v};
}

// fmtInt formats v into the tail of buf.
// It returns the index where the output begins.
int fmtInt(byte buf[], int w, uint64 v) {
    if (v == 0) {
        w--;
        buf[w] = '0';
    } else {
        for (; v > 0;) {
            w--;
            buf[w] = byte(v % 10) + '0';
            v /= 10;
        }
    }
    return w;
}

// String ...
string Duration::String() const {
    // Largest time is 2540400h10m10.000000000s
    byte buf[32] = {0};
    int w = sizeof(buf) - 1;

    uint64 u = d_;
    bool neg = d_ < 0;
    if (neg) {
        u = -u;
    }

    if (u < uint64(Second)) {
        // Special case: if duration is smaller than a second,
        // use smaller units, like 1.2ms
        int prec = 0;
        w--;
        buf[w] = 's';
        w--;
        if (u == 0) {
            return "0s";
        } else if (u < uint64(Microsecond)) {
            // print nanoseconds
            prec = 0;
            buf[w] = 'n';
        } else if (u < uint64(Millisecond)) {
            // print microseconds
            prec = 3;
            // U+00B5 'µ' micro sign == 0xC2 0xB5
            w--;  // Need room for two bytes.
            buf[w] = 'u';
        } else {
            // print milliseconds
            prec = 6;
            buf[w] = 'm';
        }
        AUTO_R(w1, u1, fmtFrac(buf, w, u, prec));
        w = w1;
        u = u1;
        w = fmtInt(buf, w, u);
    } else {
        w--;
        buf[w] = 's';

        AUTO_R(w1, u1, fmtFrac(buf, w, u, 9));
        w = w1;
        u = u1;

        // u is now integer seconds
        w = fmtInt(buf, w, u % 60);
        u /= 60;

        // u is now integer minutes
        if (u > 0) {
            w--;
            buf[w] = 'm';
            w = fmtInt(buf, w, u % 60);
            u /= 60;

            // u is now integer hours
            // Stop at hours because days can be different lengths.
            if (u > 0) {
                w--;
                buf[w] = 'h';
                w = fmtInt(buf, w, u);
            }
        }
    }

    if (neg) {
        w--;
        buf[w] = '-';
    }

    return string((char*)buf + w);
}

}  // namespace time
}  // namespace gx
