//
// weproxy@foxmail.com 2022/10/03
//

#include "io.h"

namespace gx {
namespace unitest {

// test_io ...
void test_io() {
    {
        io::ReadWriteCloser rc;

        auto _0 = io::NewCloser(rc);
        auto _1 = io::NewReader(rc);
        auto _2 = io::NewWriter(rc);
        auto _3 = io::NewReadWriter(rc);
        auto _4 = io::NewReadCloser(rc);
        auto _5 = io::NewWriteCloser(rc);
        auto _6 = io::NewReadWriteCloser(rc);
    }
    {
        auto r = [](bytez<>) -> R<int, error> { return {0, nil}; };
        auto w = [](const bytez<>) -> R<int, error> { return {0, nil}; };
        auto c = []() -> error { return nil; };

        auto _0 = io::NewCloserFn(c);
        auto _1 = io::NewReaderFn(r);
        auto _2 = io::NewWriterFn(w);
        auto _3 = io::NewReadWriterFn(r, w);
        auto _4 = io::NewReadCloserFn(r, c);
        auto _5 = io::NewWriteCloserFn(w, c);
        auto _6 = io::NewReadWriteCloserFn(r, w, c);
    }
}

}  // namespace unitest
}  // namespace gx
