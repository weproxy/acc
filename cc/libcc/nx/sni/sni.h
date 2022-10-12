//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/io/io.h"

namespace nx {
namespace sni {
using namespace gx;

// GetServerName ...
R<string, error> GetServerName(const void* buf, size_t len);

}  // namespace sni
}  // namespace nx
