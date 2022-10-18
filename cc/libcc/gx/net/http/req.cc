//
// weproxy@foxmail.com 2022/10/03
//

#include "gx/strings/strings.h"
#include "http.h"

namespace gx {
namespace http {

namespace urlpkg = url;

namespace httpguts {
static bool _isTokenTable[127];
static auto _initTokenTable = [] {
    static char SS[] = "!#$%&\'*+-.0123456789ABCDEFGHIJKLMNOPQRSTUWVXYZ^_`abcdefghijklmnopqrstuvwxyz|~";
    memset(_isTokenTable, 0, sizeof(_isTokenTable));
    for (int i = 0; i < strlen(SS); i++) {
        _isTokenTable[SS[i]] = true;
    }
    return true;
}();

static bool IsTokenRune(rune r) {
    int i = int(r);
    return i < 127 && _isTokenTable[i];
}
}  // namespace httpguts

namespace xx {
static bool isNotToken(rune r) { return !httpguts::IsTokenRune(r); }

static bool validMethod(const string& method) {
    /*
         Method         = "OPTIONS"                ; Section 9.2
                        | "GET"                    ; Section 9.3
                        | "HEAD"                   ; Section 9.4
                        | "POST"                   ; Section 9.5
                        | "PUT"                    ; Section 9.6
                        | "DELETE"                 ; Section 9.7
                        | "TRACE"                  ; Section 9.8
                        | "CONNECT"                ; Section 9.9
                        | extension-method
       extension-method = token
         token          = 1*<any CHAR except CTLs or separators>
    */
    return len(method) > 0 && strings::IndexFunc(method, isNotToken) == -1;
}

static bool hasPort(const string& s) { return strings::LastIndex(s, ":") > strings::LastIndex(s, "]"); }

static string removeEmptyPort(const string& host) {
    if (hasPort(host)) {
        return strings::TrimSuffix(host, ":");
    }
    return host;
}
}  // namespace xx

// NewRequest ...
R<Request, error> NewRequest(const string& method1, const string& url, io::Reader body) {
    const string& method = method1 == "" ? MethodGet : method1;

    if (!xx::validMethod(method)) {
        return {nil, fmt::Errorf("net/http: invalid method %s", method.c_str())};
    }

    AUTO_R(u, err, urlpkg::Parse(url));
    if (err) {
        return {nil, err};
    }

    u->Host = xx::removeEmptyPort(u->Host);

    Request req = MakeRef<xx::request_t>();
    req->Method = method;
    req->URL = u;
    req->Proto = "HTTP/1.1";
    req->ProtoMajor = 1;
    req->ProtoMinor = 1;
    req->Host = u->Host;

    return {req, nil};
}

}  // namespace http
}  // namespace gx
