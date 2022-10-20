//
// weproxy@foxmail.com 2022/10/03
//

#include "logx.h"

namespace logx {
namespace xx {

// report_t ...
static struct report_t {
    bool Enabled{false};
    std::string ServURL;
    int MaxKB{1024 * 10};
    int MaxSec{60 * 5};

    FILE* fp_{0};
    int fsize_{0};
    int findex_{0};

    // checkFile ...
    void checkFile() {
        if (!fp_) {
        }
    }
} _report;

// report ...
void report(const std::string& msg) {
    if (!_report.Enabled) {
        return;
    }
}

}  // namespace xx
}  // namespace logx
