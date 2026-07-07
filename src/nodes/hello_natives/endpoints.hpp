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

struct graph_cell {  // a graph-valued source (the structured lane)
  au::in<au::text, au::value> name;
  au::out<au::graph, au::value> out;
};

struct node_count {  // pure structured transform: graph in, text out
  au::in<au::graph, au::value> in;
  au::out<au::text, au::value> out;
};

struct text_cell {
  au::in<au::text, au::value> value;
  au::out<au::text, au::value> out;
};

struct nan_bomb {
  au::in<au::scalar, au::value> at;
  au::out<au::audio, au::block> out;
};

struct spin {
  au::in<au::audio, au::block> in;
  au::in<au::scalar, au::value> iters;
  au::out<au::audio, au::block> out;
};

struct sleeper {
  au::out<au::scalar, au::value> out;
};

struct spanv {  // a span-valued source: its values list drives lifting
  au::out<au::span, au::value> out;
};

struct mix {  // consumes a span of audio WHOLE: an N-ary sum, no clones
  au::in<au::span, au::block> in;
  au::out<au::audio, au::block> out;
};

struct instanced_draw {  // the draw boundary consumes the span whole;
  // it PRESENTS when the head's chain reaches it (PKG-4: draw order is
  // the event chain from render_head; unchained draws never render)
  au::in<au::span, au::value> instances;
  au::in<au::bang, au::event> tick;
  au::out<au::scalar, au::value> calls;
  au::out<au::scalar, au::value> drawn;
  au::out<au::bang, au::event> chain;
};

struct net_proxy {  // the net package's consumer-side proxy: a remote
  // value arrives as coalescable update events (PKG-6, the bang-with-
  // payload idiom); the proxy holds the LATEST and republishes it on the
  // value lane
  au::in<au::bang, au::event> in;
  au::out<au::scalar, au::value> out;
};

struct render_head {  // the render package's published frame clock
  // (ADR-015: clocks are INPUTS — "always dirty" is visible dataflow)
  au::out<au::bang, au::event> frame;
};

struct mesh_from_spans {  // a mesh constructor (CORE): a list of NDC vertex
  // positions (delivered as a serialized list default, read by set_text)
  // translated by dx/dy -> one structured `mesh` value (Phase A / PKG-4.3).
  // v1 reads a positions default; span-EDGE-fed geometry is a later
  // succession (a runtime span svalue), recorded in STRENGTHENINGS.
  au::in<au::span, au::value> positions;
  au::in<au::scalar, au::value> dx;
  au::in<au::scalar, au::value> dy;
  au::out<au::mesh, au::value> out;
};

struct surface_flat {  // a surface constructor (CORE): flat RGBA -> a
  // structured `surface` value naming the built-in `flat` program
  au::in<au::scalar, au::value> r;
  au::in<au::scalar, au::value> g;
  au::in<au::scalar, au::value> b;
  au::in<au::scalar, au::value> a;
  au::out<au::surface, au::value> out;
};

struct draw {  // the render boundary (RENDER pkg): holds the latest
  // mesh+surface value pair (svalue_tick) and presents when the head's
  // chain reaches it (sapply 'tick' -> semit 'chain', verbatim from
  // instanced_draw — pkg42). The render_region executor reads the held
  // pair and issues GL; the node itself stays passive (ADR-015, L9).
  au::in<au::mesh, au::value> mesh;
  au::in<au::surface, au::value> surface;
  au::in<au::bang, au::event> tick;
  au::out<au::bang, au::event> chain;
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

// host-bound terminal placeholder (FRZ-2's tier-3 culprit): a node whose
// realization needs a host OS — the freezer names it when it taints a tier
struct tmux {
  au::in<au::scalar, au::value> in;
  au::out<au::scalar, au::value> out;
};

struct scale {
  au::in<au::scalar, au::value> in;
  au::in<au::scalar, au::value> k;
  au::out<au::scalar, au::value> out;
};

}  // namespace syg::nodes::decl
