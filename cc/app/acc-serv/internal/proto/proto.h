//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../def.h"
#include "gx/encoding/json/json.h"
#include "gx/io/io.h"

namespace internal {
namespace proto {

////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[proto]";

////////////////////////////////////////////////////////////////////////////////
// server_t ...
struct server_t : public io::xx::closer_t {
    virtual error Start() = 0;
    virtual error Close() = 0;
};

// Server ...
using Server = Ref<server_t>;

// NewServerFn ...
using NewServerFn = func<R<Server, error>(const json::J& j)>;

////////////////////////////////////////////////////////////////////////////////
// Register ...
void Register(const string& proto, const NewServerFn& fn);

////////////////////////////////////////////////////////////////////////////////
// Init ..
error Init(const json::J& j);

////////////////////////////////////////////////////////////////////////////////
// Deinit ...
void Deinit();

}  // namespace proto
}  // namespace internal
