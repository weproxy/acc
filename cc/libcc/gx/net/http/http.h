//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/builtin/builtin.h"
#include "gx/errors/errors.h"

namespace gx {
namespace http {

// Errors used by the HTTP server.
extern const error ErrBodyNotAllowed;
extern const ErrHijacked;
extern const ErrContentLength;
extern const ErrWriteAfterFlush;

}  // namespace http
}  // namespace gx
