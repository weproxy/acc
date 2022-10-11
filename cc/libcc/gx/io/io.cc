//
// weproxy@foxmail.com 2022/10/03
//

#include "io.h"

#include "errors/errors.h"

namespace gx {
namespace io {

// error ...
const error ErrShortWrite = errors::New("short write");
const error errInvalidWrite = errors::New("invalid write result");
const error ErrShortBuffer = errors::New("short buffer");
const error ErrEOF = errors::New("EOF");
const error ErrUnexpectedEOF = errors::New("unexpected EOF");
const error ErrNoProgress = errors::New("multiple Read calls return no data or error");

// Discard ...
xx::discard_t Discard;

}  // namespace io
}  // namespace gx
