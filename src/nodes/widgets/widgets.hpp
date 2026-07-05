// clause: scaffolding (dissolves: CNF-4) — ABI-1.1's demonstration pair; a real kind succession (kind-v2) carries the same assertion
#pragma once
#include "ports.hpp"

namespace syg::nodes {
namespace au = syg::authoring;

// widget_b is widget_a plus EXACTLY ONE declaration line (`brightness`).
// ABI-1.1: that one line must surface the port, the codec field, and the
// binding field with no other edits anywhere.

struct widget_a {
  au::in<au::scalar, au::value> gain;
  au::in<au::audio, au::block> input;
  au::out<au::audio, au::block> output;
};

struct widget_b {
  au::in<au::scalar, au::value> gain;
  au::in<au::scalar, au::value> brightness;
  au::in<au::audio, au::block> input;
  au::out<au::audio, au::block> output;
};

}  // namespace syg::nodes
