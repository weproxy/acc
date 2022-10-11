//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "util.h"

#define FX_SS(...)                \
    [&] {                         \
        std::ostringstream _s2s_; \
        _s2s_ << __VA_ARGS__;     \
        return _s2s_.str();       \
    }()
