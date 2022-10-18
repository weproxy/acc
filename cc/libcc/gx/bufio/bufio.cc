//
// weproxy@foxmail.com 2022/10/03
//

#include "bufio.h"

namespace gx {
namespace bufio {

const error ErrInvalidUnreadByte = errors::New("bufio: invalid use of UnreadByte");
const error ErrInvalidUnreadRune = errors::New("bufio: invalid use of UnreadRune");
const error ErrBufferFull = errors::New("bufio: buffer full");
const error ErrNegativeCount = errors::New("bufio: negative count");

const error errNegativeRead = errors::New("bufio: reader returned negative count from Read");

}  // namespace bufio
}  // namespace gx
