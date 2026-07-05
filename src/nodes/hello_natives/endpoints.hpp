// clause: floor — the endpoints declarations for the starter vocabulary
// (AUT-3: ports, promises, codecs, bindings all generate from these)
#pragma once
#include "ports.hpp"

namespace syg::nodes::decl {
namespace au = syg::authoring;

struct osc {
  au::in<au::scalar, au::value> freq;
  au::in<au::scalar, au::value> shape;
  au::out<au::audio, au::block> out;
};

struct lfo {
  au::in<au::scalar, au::value> freq;
  au::in<au::scalar, au::value> depth;
  au::out<au::scalar, au::frame> out;
};

struct vca {
  au::in<au::audio, au::block> in;
  au::in<au::scalar, au::block> gain;
  au::out<au::audio, au::block> out;
};

struct dac {
  au::in<au::audio, au::block> in;
};

struct noise {
  au::out<au::audio, au::block> out;
};

struct add {
  au::in<au::audio, au::block> a;
  au::in<au::audio, au::block> b;
  au::out<au::audio, au::block> out;
};

struct cell {
  au::in<au::scalar, au::value> k;
  au::out<au::scalar, au::value> out;
};

struct scale {
  au::in<au::scalar, au::value> in;
  au::in<au::scalar, au::value> k;
  au::out<au::scalar, au::value> out;
};

}  // namespace syg::nodes::decl
