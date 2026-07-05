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

struct delay {
  au::in<au::audio, au::block> in;
  au::in<au::scalar, au::value> samples;
  au::out<au::audio, au::block> out;
};

struct pulse {
  au::out<au::audio, au::block> out;
};

struct spectro {  // block-override: consumes its block whole (FFT-shaped)
  au::in<au::audio, au::block> in;
  au::out<au::audio, au::block> out;
};

struct spanv {  // a span-valued source: its values list drives lifting
  au::out<au::span, au::value> out;
};

struct mix {  // consumes a span of audio WHOLE: an N-ary sum, no clones
  au::in<au::span, au::block> in;
  au::out<au::audio, au::block> out;
};

struct instanced_draw {  // the draw boundary consumes the span whole
  au::in<au::span, au::value> instances;
  au::out<au::scalar, au::value> calls;
  au::out<au::scalar, au::value> drawn;
};

struct graph_source {  // the reflection seam: sees its enclosing graph
  au::out<au::scalar, au::value> keys;
};

struct smoother {  // a user-suppliable boundary mapping (EXE-8, CMP-4)
  au::in<au::scalar, au::value> in;
  au::in<au::scalar, au::value> rate;
  au::out<au::scalar, au::block> out;
};

struct button {
  au::out<au::bang, au::event> out;
};

struct counter {
  au::in<au::bang, au::event> in;
  au::out<au::scalar, au::value> out;
  au::out<au::scalar, au::value> errors;
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
