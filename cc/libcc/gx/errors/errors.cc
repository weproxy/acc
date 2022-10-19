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
error New(const string& s) { return NewRef<strerr_t>(s); }

}  // namespace errors
}  // namespace gx
