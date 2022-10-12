//
// weproxy@foxmail.com 2022/10/03
//

#include "logx.h"

namespace logx {
namespace xx {

// report ...
static struct report {
    bool enable_;
    std::string servUrl_;
    int maxKB_;
    int maxSec_;

    FILE* fp_;
    int fsize_;
    int findex_;

    report() : enable_(false), maxKB_(1024 * 2), maxSec_(60 * 5), fp_(0), fsize_(0), findex_(0) {}

    void checkFile() {
        if (!fp_) {
        }
    }
} _report;

// _report ...
void report(const std::string& msg) {
    if (!_report.enable_) {
        return;
    }
}

}  // namespace xx
}  // namespace logx
