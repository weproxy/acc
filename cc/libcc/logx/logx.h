//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <sstream>

#include "gx/gx.h"

////////////////////////////////////////////////////////////////////////////////
//
namespace logx {
using namespace gx;

// Level ..
enum class Level {
    NONE = 0,
    ERR,
    WRN,
    MSG,
    DBG,
    MAX,
};

namespace xx {

// reach ...
extern bool reach(Level lvl);

// logs ...
extern void logs(Level lvl, const char* file, int line, const char* msg);

// logf ...
extern void logf(Level lvl, const char* file, int line, const char* fmt, va_list args);
extern void logf(Level lvl, const char* file, int line, const char* fmt, ...);

// report ...
extern void report(const std::string& msg);

// out ...
inline void out(std::ostream&) {}

template <typename T, typename... X>
void out(std::ostream& s, T&& t, X&&... x) {
    s << std::forward<T>(t) << " ";
    out(s, std::forward<X>(x)...);
}

template <typename... T>
void out(std::ostream& s, T&&... t) {
    out(s, std::forward<T>(t)...);
}

// logt ...
template <typename... T>
void logt(Level lvl, const char* file, int line, T&&... t) {
    std::ostringstream s;
    out(s, std::forward<T>(t)...);
    logs(lvl, file, line, s.str().c_str());
}

}  // namespace xx

////////////////////////////////////////////////////////////////////////////////
//
template <typename... X>
void V(X&&... x) {
    xx::logt(Level::MAX, 0, 0, std::forward<X>(x)...);
}

template <typename... X>
void D(X&&... x) {
    xx::logt(Level::DBG, 0, 0, std::forward<X>(x)...);
}

template <typename... X>
void I(X&&... x) {
    xx::logt(Level::MSG, 0, 0, std::forward<X>(x)...);
}

template <typename... X>
void W(X&&... x) {
    xx::logt(Level::WRN, 0, 0, std::forward<X>(x)...);
}

template <typename... X>
void E(X&&... x) {
    xx::logt(Level::ERR, 0, 0, std::forward<X>(x)...);
}

template <typename... X>
void P(X&&... x) {
    xx::logt(Level::NONE, 0, 0, std::forward<X>(x)...);
}
}  // namespace logx

////////////////////////////////////////////////////////////////////////////////
//
#define _LOGS_T_(LEVEL, ...)                                    \
    do {                                                        \
        if (logx::xx::reach(LEVEL)) {                           \
            std::ostringstream s;                               \
            s << __VA_ARGS__;                                   \
            logx::xx::logt(LEVEL, __FILE__, __LINE__, s.str()); \
        }                                                       \
    } while (0)

#define _LOGX_T_(LEVEL, ...)                                        \
    do {                                                            \
        if (logx::xx::reach(LEVEL)) {                               \
            logx::xx::logt(LEVEL, __FILE__, __LINE__, __VA_ARGS__); \
        }                                                           \
    } while (0)

#define _LOGF_T_(LEVEL, fmt, ...)                                        \
    do {                                                                 \
        if (logx::xx::reach(LEVEL)) {                                    \
            logx::xx::logf(LEVEL, __FILE__, __LINE__, fmt, __VA_ARGS__); \
        }                                                                \
    } while (0)

// Global ...
//    LOGS_D("A" << 1 << 2.3f);
#define LOGS_V(...) _LOGS_T_(logx::Level::MAX, __VA_ARGS__)
#define LOGS_D(...) _LOGS_T_(logx::Level::DBG, __VA_ARGS__)
#define LOGS_I(...) _LOGS_T_(logx::Level::MSG, __VA_ARGS__)
#define LOGS_W(...) _LOGS_T_(logx::Level::WRN, __VA_ARGS__)
#define LOGS_E(...) _LOGS_T_(logx::Level::ERR, __VA_ARGS__)
#define LOGS_P(...) _LOGS_T_(logx::Level::NONE, __VA_ARGS__)

// Global ...
//    LOGX_D("A", 1, 2.3f);
#define LOGX_V(...) _LOGX_T_(logx::Level::MAX, __VA_ARGS__)
#define LOGX_D(...) _LOGX_T_(logx::Level::DBG, __VA_ARGS__)
#define LOGX_I(...) _LOGX_T_(logx::Level::MSG, __VA_ARGS__)
#define LOGX_W(...) _LOGX_T_(logx::Level::WRN, __VA_ARGS__)
#define LOGX_E(...) _LOGX_T_(logx::Level::ERR, __VA_ARGS__)
#define LOGX_P(...) _LOGX_T_(logx::Level::NONE, __VA_ARGS__)

// Global ..
//    LOGF_D("%s %d %f", "A", 1, 2.3f);
#define LOGF_V(fmt, ...) _LOGF_T_(logx::Level::MAX, fmt, __VA_ARGS__)
#define LOGF_D(fmt, ...) _LOGF_T_(logx::Level::DBG, fmt, __VA_ARGS__)
#define LOGF_I(fmt, ...) _LOGF_T_(logx::Level::MSG, fmt, __VA_ARGS__)
#define LOGF_W(fmt, ...) _LOGF_T_(logx::Level::WRN, fmt, __VA_ARGS__)
#define LOGF_E(fmt, ...) _LOGF_T_(logx::Level::ERR, fmt, __VA_ARGS__)
#define LOGF_P(fmt, ...) _LOGF_T_(logx::Level::NONE, fmt, __VA_ARGS__)
