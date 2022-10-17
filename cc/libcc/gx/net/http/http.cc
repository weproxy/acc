//
// weproxy@foxmail.com 2022/10/03
//

#include "http.h"

#include "gx/errors/errors.h"

namespace gx {
namespace http {

// Errors used by the HTTP server.
const error ErrBodyNotAllowed = errors::New("http: request method or response status code does not allow body");
const error ErrHijacked = errors::New("http: connection has been hijacked");
const error ErrContentLength = errors::New("http: wrote more than the declared Content-Length");
const error ErrWriteAfterFlush = errors::New("unused");

}  // namespace http
}  // namespace gx
