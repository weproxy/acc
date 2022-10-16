//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../def.h"
#include "gx/io/io.h"
#include "gx/encoding/json/json.h"

namespace app {
namespace proto {

////////////////////////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[proto]";

////////////////////////////////////////////////////////////////////////////////////////////////////
// IServer ...
struct IServer : public io::xx::closer_t {
    virtual error Start() = 0;
    virtual void Close() = 0;
};

// Server ...
typedef std::shared_ptr<IServer> Server;

// NewServerFn ...
typedef std::function<R<Server, error>(const json::J& j)> NewServerFn;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Register ...
void Register(const string& proto, const NewServerFn& fn);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Init ..
error Init(const json::J& j);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Deinit ...
void Deinit();

}  // namespace proto
}  // namespace app
