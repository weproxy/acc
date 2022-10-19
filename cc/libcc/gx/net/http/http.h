//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/errors/errors.h"
#include "gx/io/io.h"
#include "gx/net/url/url.h"

namespace gx {
namespace http {

// Unless otherwise noted, these are defined in RFC 7231 section 4.3.
const char* MethodGet = "GET";
const char* MethodHead = "HEAD";
const char* MethodPost = "POST";
const char* MethodPut = "PUT";
const char* MethodPatch = "PATCH";  // RFC 5789
const char* MethodDelete = "DELETE";
const char* MethodConnect = "CONNECT";
const char* MethodOptions = "OPTIONS";
const char* MethodTrace = "TRACE";

// Errors used by the HTTP server.
extern const error ErrBodyNotAllowed;
extern const error ErrHijacked;
extern const error ErrContentLength;
extern const error ErrWriteAfterFlush;

// Header ...
struct Header : public url::Values {
    slice<string> Values(const string& key) { return map_[key]; }

    Header Clone() { return {}; }
};

// Response ...
struct Response {
    // ...
};

// ResponseWriter ...
struct ResponseWriter {
    // ...
};

// GetBodyFn ...
using GetBodyFn = func<R<io::ReadCloser, error>()>;

// Request ...
struct Request {
    string Method;      // GET, POST, PUT, etc.
    Ref<url::URL> URL;  // https://xxx etc.

    string Proto;       // "HTTP/1.0"
    int ProtoMajor{0};  // 1
    int ProtoMinor{0};  // 0

    Header Header;

    io::ReadCloser Body;

    GetBodyFn GetBody;

    int64 ContentLength{0};

    slice<string> TransferEncoding;

    bool Close{false};

    string Host;

    url::Values Form;
    url::Values PostForm;

    struct Header Trailer;

    string RemoteAdr;
    string RequestURI;

    Response Response;
};

////////////////////////////////////////////////////////////////////////////////
//
// NewRequest ...
R<Ref<Request>, error> NewRequest(const string& method, const string& url, io::Reader body);

}  // namespace http
}  // namespace gx
