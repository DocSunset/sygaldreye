// clause: scaffolding (dissolves: CNF-1) — ABI-2/3 audit nodes; retire when the suite runs as datasets and real fallible natives carry these assertions
#pragma once
#include "ports.hpp"

namespace syg::nodes {
namespace au = syg::authoring;

struct parsey {  // declared fallible: carries a fault out-port (ABI-3.1)
  au::in<au::scalar, au::block> input;
  au::out<au::scalar, au::block> output;
  au::out<au::fault, au::event> fault;
};
void parsey_body(const float* in, float* out, int frames);  // throws on malformed (negative) input

struct boom {  // noexcept-by-contract: no fault port, so no shell handler (ABI-3.2)
  au::in<au::scalar, au::block> input;
  au::out<au::scalar, au::block> output;
};
void boom_body(const float* in, float* out, int frames);  // undeclared throw on input > 0.5

struct devicey {  // resource holder done right: prepare acquires (ABI-2.2)
  au::in<au::scalar, au::block> input;
  au::out<au::scalar, au::block> output;
};
void devicey_prepare_body(int rate, int max_block);

struct devicey_bad {  // the violation: create acquires (ABI-2.2)
  au::in<au::scalar, au::block> input;
  au::out<au::scalar, au::block> output;
};
void devicey_bad_create_body();

}  // namespace syg::nodes
