//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../def.h"
#include "nx/stack/stack.h"

namespace app {
namespace proto {
using nx::stack::Handler;
using nx::stack::handler_t;

////////////////////////////////////////////////////////////////////////////////
// TAG ...
constexpr const char* TAG = "[proto]";

// NewHandlerFn ...
using NewHandlerFn = func<R<Handler, error>(const string& servURL)>;

////////////////////////////////////////////////////////////////////////////////
// Register ...
void Register(const string& proto, const NewHandlerFn& fn);

// GetHandler ...
R<Handler, error> GetHandler(const string& servURL);

////////////////////////////////////////////////////////////////////////////////
// Init ..
error Init();

////////////////////////////////////////////////////////////////////////////////
// Deinit ...
void Deinit();

}  // namespace proto
}  // namespace app
