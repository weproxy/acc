//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/gx.h"
#include "gx/errors/errors.h"

namespace gx {
namespace http {

// Errors used by the HTTP server.
const error ErrBodyNotAllowed = errors::New("http: request method or response status code does not allow body");
const error ErrHijacked = errors::New("http: connection has been hijacked");
const error ErrContentLength = errors::New("http: wrote more than the declared Content-Length");
const error ErrWriteAfterFlush = errors::New("unused");

// Header ...
struct Header {
    typedef MapPtr<string, slice<string>> Values;
    Values map_;

    Header& Add(const string& key, const string& val) {
        // Header
        return *this;
    }
};

namespace xx {

// responseWriter_t ...
struct responseWriter_t {

};

// ResponseWriter ...
typedef SharedPtr<responseWriter_t> ResponseWriter;
}  // namespace xx

// ResponseWriter ...
using xx::ResponseWriter;

}  // namespace http
}  // namespace gx
