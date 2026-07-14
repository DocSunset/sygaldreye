#pragma once
#include "binding.hpp"
#include <span>

namespace syg::esc {

// The escapement: tick each binding in order. The movement is a span of pointers
// to self-owning bindings (each mallocs its own blob in make_binding), so we tick
// through the indirection.


void tick(std::span<binding* const> movement) {
  for (binding* b : movement) (*b)();
}

}
