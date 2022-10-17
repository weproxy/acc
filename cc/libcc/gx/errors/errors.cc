//
// weproxy@foxmail.com 2022/10/03
//

#include "errors.h"

namespace gx {
namespace errors {

// strerr_t ...
struct strerr_t : public err_t {
    string s_;
    strerr_t(const string& s) : s_(s) {}
    virtual string String() const override { return s_; }
};

// New ...
error New(const string& fmt, ...) {
    string s;

    va_list ap;
    va_start(ap, fmt);

    // vasprintf 会自动分配，调用者负责释放
    char* ptr = 0;
    int len = vasprintf(&ptr, fmt.c_str(), ap);
    if (ptr) {
        s.assign(ptr, len);
        std::free(ptr);
    }

    va_end(ap);

    return SharedPtr<strerr_t>(new strerr_t(s));
}

}  // namespace errors
}  // namespace gx
