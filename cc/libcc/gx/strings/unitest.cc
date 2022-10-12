//
// weproxy@foxmail.com 2022/10/03
//

#include "strings.h"

namespace gx {
namespace unitest {
void test_strings() {
    string s = " av XX av ";
    std::cout << "s =" << s << ";" << std::endl;

    std::cout << "strings::TrimLeft(s, \" \\t\") =" << strings::TrimLeft(s, " \t") << ";" << std::endl;
    std::cout << "strings::TrimRight(s, \" \\r\\n\") =" << strings::TrimRight(s, " \r\n") << ";" << std::endl;
    std::cout << "strings::TrimSpace(s) =" << strings::TrimSpace(s) << ";" << std::endl;
    std::cout << "strings::Trim(s, \" \") =" << strings::Trim(s, " ") << ";" << std::endl;
    std::cout << "strings::Count(s, \"av\") =" << strings::Count(s, "av") << ";" << std::endl;
    std::cout << "strings::Index(s, \"av\") =" << strings::Index(s, "av") << ";" << std::endl;
    std::cout << "strings::LastIndex(s, \"av\") =" << strings::LastIndex(s, "av") << ";" << std::endl;
    std::cout << "strings::TrimPrefix(s, \" av \") =" << strings::TrimPrefix(s, " av ") << ";" << std::endl;
    std::cout << "strings::TrimSuffix(s, \" av \") =" << strings::TrimSuffix(s, " av ") << ";" << std::endl;

    auto v = strings::Split(s, "XX");
    std::ostringstream ss;
    ss << "{";
    bool b = 0;
    for (auto& c : v) {
        if (!b) {
            b = 1;
        } else {
            ss << ", ";
        }
        ss << c;
    }
    ss << "}";

    std::cout << "strings::Split(s, \"XX\") =" << v << ";" << std::endl;
}
}  // namespace unitest
}  // namespace gx
