// clause: fixture (ADR-036) — the ABI-1.1 demonstration pair (widget_b is
// widget_a plus one declaration line: `brightness`). Permanent: it is the
// fixture rung 2's ABI-1.1 test consumes (descriptors, codec-selftest,
// bindings), not scaffolding. The port-addition it demonstrates classifies as
// an ADDITIVE kind succession (CNF-4 cross-checks the descriptor delta), but
// its permanence comes from ABI-1.1, not from any dissolution.
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
