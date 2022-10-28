//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "../def.h"
#include "nx/stack/stack.h"

namespace internal {
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
void Register(const stringz<>& protos, const NewHandlerFn& fn);

// GetHandler ...
R<Handler, error> GetHandler(const string& servURL);

////////////////////////////////////////////////////////////////////////////////
// Init ..
error Init();

////////////////////////////////////////////////////////////////////////////////
// Deinit ...
void Deinit();

}  // namespace proto
}  // namespace internal
