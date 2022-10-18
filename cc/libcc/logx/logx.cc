//
// weproxy@foxmail.com 2022/10/03
//

#include "logx.h"

#include <sys/time.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
namespace logx {
static struct global_init {
    global_init() : oldMode_(0) {
        GetConsoleMode(stdout, &oldMode_);
        SetConsoleMode(stdout, oldMode_ | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    ~global_init() {
        if (oldMode_) {
            SetConsoleMode(stdout, oldMode_);
        }
    }

    int oldMode_;
} _global_init;
}  // namespace logx
#endif  // _WIN32

namespace logx {

#ifndef _TCLEAN_
#define _TCLEAN_ "\033[0m"
#define _TWEAK_ "\033[2m"
#define _TULINE_ "\033[4m"

#define _TBLACK_ "\033[30m"
#define _TRED_ "\033[31m"
#define _TGREEN_ "\033[32m"
#define _TYELLOW_ "\033[33m"
#define _TBLUE_ "\033[34m"
#define _TMAGENTA_ "\033[35m"
#define _TCYAN_ "\033[36m"
#define _TWHITE_ "\033[37m"
#endif

namespace xx {
// conf ...
static struct conf {
    Level stdoutLevel;
    Level reportLevel;
    bool showCaller;
    bool showDatetime;
    bool withColor;

    conf() : stdoutLevel(Level::DBG), reportLevel(Level::DBG), showCaller(true), showDatetime(true), withColor(true) {}
} _conf;

static const char* _tags[] = {"[P] ", "[E] ", "[W] ", "[I] ", "[D] ", "[V] "};
static const char* _colors[] = {_TCYAN_, _TRED_, _TYELLOW_, _TGREEN_, _TWHITE_, _TMAGENTA_};

// _reach ...
bool reach(Level lvl) { return _conf.stdoutLevel >= lvl || _conf.reportLevel >= lvl; }

// timestr ...
static std::string timestr() {
    struct tm t;
    struct timeval tv;

    ::gettimeofday(&tv, 0);
    time_t v = tv.tv_sec;

    ::localtime_r(&v, &t);

    char buf[128];
    int len = ::snprintf(buf, sizeof(buf), "%02d-%02d %02d:%02d:%02d.%06d", t.tm_mon + 1, t.tm_mday, t.tm_hour,
                         t.tm_min, t.tm_sec, (int)tv.tv_usec);

    return std::string(buf, len);
}

// logs ...
void logs(Level lvl, const char* file, int line, const char* msg) {
    if (!msg || !msg[0]) return;
    if (lvl < Level::NONE || lvl > Level::MAX) {
        lvl = Level::NONE;
    }

    std::ostringstream ss;

    if (_conf.withColor) {
        ss << _TCLEAN_ << _TWEAK_;
    }
    if (_conf.showDatetime) {
        ss << timestr();
    }
    ss << _tags[lvl];
    if (_conf.showCaller) {
        ss << file << ":" << line << " ";
    }
    if (_conf.withColor) {
        ss << _TCLEAN_ << _colors[lvl];
    }
    ss << msg << std::endl;
    if (_conf.withColor) {
        ss << _TCLEAN_;
    }

    std::string str = ss.str();

    if (_conf.stdoutLevel >= lvl) {
        std::cout << str;
    }

    if (_conf.reportLevel >= lvl) {
        report(str);
    }
}

// logf ...
void logf(Level lvl, const char* file, int line, const char* fmt, ...) {
    if (!fmt || !fmt[0]) return;

    if (reach(lvl)) {
        va_list ap;
        va_start(ap, fmt);
        logf(lvl, file, line, fmt, ap);
        va_end(ap);
    }
}

// logf ...
void logf(Level lvl, const char* file, int line, const char* fmt, va_list args) {
    // vasprintf 会自动分配，调用者负责释放
    char* ptr = 0;
    vasprintf(&ptr, fmt, args);
    if (ptr) {
        logs(lvl, file, line, ptr);
        std::free(ptr);
    }
}

}  // namespace xx
}  // namespace logx
