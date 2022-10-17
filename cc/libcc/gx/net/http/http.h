//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/errors/errors.h"
#include "gx/io/io.h"
#include "gx/net/url/url.h"

namespace gx {
namespace http {

// Errors used by the HTTP server.
extern const error ErrBodyNotAllowed;
extern const error ErrHijacked;
extern const error ErrContentLength;
extern const error ErrWriteAfterFlush;

namespace xx {
// header_t ...
struct header_t {
    // ...
};

// Header ...
using Header = SharedPtr<header_t>;

// request_t ...
struct request_t {
    string Method;  // GET, POST, PUT, etc.
    url::URL URL;   // https://xxx etc.

    string Proto;       // "HTTP/1.0"
    int ProtoMajor{0};  // 1
    int ProtoMinor{0};  // 0

    Header Header;

    io::ReadCloser Body;

    std::function<R<io::ReadCloser, error>()> GetBody;

    int64 ContentLength{0};

    slice<string> TransferEncoding;

    bool Close{false};

    string Host;

};

};  // namespace xx

// Request ...
using Request = SharedPtr<xx::request_t>;

}  // namespace http
}  // namespace gx
