//
// weproxy@foxmail.com 2022/10/03
//

#include "hex.h"

namespace gx {
namespace unitest {
void test_hex() {
    const string s = "hello";
    std::cout << "s =" << s << std::endl;

    string s1 = hex::EncodeToString(s);
    std::cout << "hex::EncodeToString(s) =" << s1 << std::endl;

    AUTO_R(s2, err, hex::DecodeString(s1));
    if (err) {
        std::cout << "hex::DecodeString(s1) err: " << err << std::endl;
    } else {
        std::cout << "hex::DecodeString(s1) =" << string((char*)s2.data(), s2.size()) << std::endl;
    }
}
}  // namespace unitest
}  // namespace gx
