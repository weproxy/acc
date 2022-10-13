//
// weproxy@foxmail.com 2022/10/03
//

#include "fmt.h"

#include "gx/errors/errors.h"

namespace gx {
namespace fmt {

// format ...
static string format(const string& fmt, va_list& ap) {
    string s;

    // vasprintf 会自动分配，调用者负责释放
    char* ptr = 0;
    int len = vasprintf(&ptr, fmt.c_str(), ap);
    if (ptr) {
        s.assign(ptr, len);
        std::free(ptr);
    }

    return s;
}

// Sprintf ...
string Sprintf(const string& fmt, ...) {
    if (fmt.empty()) {
        return "";
    }

    va_list ap;
    va_start(ap, fmt);
    string s = format(fmt, ap);
    va_end(ap);

    return s;
}

// Errorf ...
error Errorf(const string& fmt, ...) {
    if (fmt.empty()) {
        return errors::New("error");
    }

    va_list ap;
    va_start(ap, fmt);
    string s = format(fmt, ap);
    va_end(ap);

    return errors::New(s);
}

}  // namespace fmt
}  // namespace gx
