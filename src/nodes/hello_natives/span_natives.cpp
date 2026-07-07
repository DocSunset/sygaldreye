// clause: floor — the span vocabulary: spanv (a span-valued source; its
// list default drives lift expansion), mix (whole-by-kind N-ary sum), and
// instanced_draw (a headless stand-in for the render boundary consuming a
// span whole — the GL reality arrives with the render package, rung 8;
// scaffolding-adjacent but the SEMANTICS here are permanent)
#include "crown.hpp"

#include <cstring>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "native_ports.hpp"
#include "svalue_accessors.hpp"

namespace syg::nodes {
namespace {

struct n_state {
  int n = 0;
};
void n_set(void* s, const char* port, double v) {
  if (!std::strcmp(port, "n")) static_cast<n_state*>(s)->n = int(v);
}
void mix_process(void* s, const float* const* in, float* const* out,
                 int frames) noexcept {
  auto* st = static_cast<n_state*>(s);
  // AUT-2 exception: span gather machinery
  for (int i = 0; i < frames; ++i) {
    float acc = 0.0f;
    for (int k = 0; k < st->n; ++k) acc += in[k][i];
    out[0][i] = acc;
  }
}

struct draw_state {
  int n = 0;
  long calls = 0;
  int pending = 0;
};
void draw_set(void* s, const char* port, double v) {
  if (!std::strcmp(port, "n")) static_cast<draw_state*>(s)->n = int(v);
}
// the present happens when the HEAD'S CHAIN reaches this draw (PKG-4.2):
// one call per chain bang, however many instances; the bang forwards so
// draws downstream in the chain present the same frame, in wiring order
void draw_sapply(void* s, const char* port, const syg::crown::svalue&) {
  if (std::strcmp(port, "tick")) return;
  auto* st = static_cast<draw_state*>(s);
  ++st->calls;
  ++st->pending;
}
bool draw_semit(void* s, const char* port, syg::crown::svalue* out) {
  if (std::strcmp(port, "chain")) return false;
  auto* st = static_cast<draw_state*>(s);
  if (!st->pending) return false;
  --st->pending;
  *out = {"bang", nullptr};
  return true;
}
void draw_value_tick(void* s, double, const float*, float* outs) {
  auto* st = static_cast<draw_state*>(s);
  outs[0] = static_cast<float>(st->calls);
  outs[1] = static_cast<float>(st->n);
}

struct head_state {
  int pending = 0;
};
void head_value_tick(void* s, double, const float*, float*) {
  ++static_cast<head_state*>(s)->pending;  // one frame bang per frame tick
}
bool head_semit(void* s, const char* port, syg::crown::svalue* out) {
  if (std::strcmp(port, "frame")) return false;
  auto* st = static_cast<head_state*>(s);
  if (!st->pending) return false;
  --st->pending;
  *out = {"bang", nullptr};
  return true;
}
void no_process(void*, const float* const*, float* const*, int) noexcept {}

// --- surface_flat: flat RGBA -> a `surface` structured value (PKG-4.3) ---
struct surface_flat_state {
  float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
};
void sf_set(void* s, const char* port, double v) {
  auto* st = static_cast<surface_flat_state*>(s);
  if (!std::strcmp(port, "r")) st->r = static_cast<float>(v);
  else if (!std::strcmp(port, "g")) st->g = static_cast<float>(v);
  else if (!std::strcmp(port, "b")) st->b = static_cast<float>(v);
  else if (!std::strcmp(port, "a")) st->a = static_cast<float>(v);
}
void sf_svalue_tick(void* s, const syg::crown::svalue*,
                    syg::crown::svalue* outs) {
  auto* st = static_cast<surface_flat_state*>(s);
  syg::generated::surface_data sd;
  sd.r = st->r; sd.g = st->g; sd.b = st->b; sd.a = st->a;
  sd.program = "flat";
  outs[0] = syg::generated::make_surface(std::move(sd));
}

// --- mesh_from_spans: a positions list (a serialized list default, read by
// set_text) translated by dx/dy -> a `mesh` structured value (PKG-4.3) ---
struct mesh_from_spans_state {
  std::vector<float> base;  // interleaved x,y in NDC (2 per vertex)
  float dx = 0.0f, dy = 0.0f;
};
void mfs_set(void* s, const char* port, double v) {
  auto* st = static_cast<mesh_from_spans_state*>(s);
  if (!std::strcmp(port, "dx")) st->dx = static_cast<float>(v);
  else if (!std::strcmp(port, "dy")) st->dy = static_cast<float>(v);
}
void mfs_set_text(void* s, const char* port, const char* v) {
  if (std::strcmp(port, "positions")) return;
  auto* st = static_cast<mesh_from_spans_state*>(s);
  st->base.clear();
  auto j = nlohmann::json::parse(v, nullptr, false);
  if (j.is_array())
    for (const auto& p : j)
      if (p.is_array() && p.size() >= 2) {
        st->base.push_back(p[0].get<float>());
        st->base.push_back(p[1].get<float>());
      }
}
void mfs_svalue_tick(void* s, const syg::crown::svalue*,
                     syg::crown::svalue* outs) {
  auto* st = static_cast<mesh_from_spans_state*>(s);
  syg::generated::mesh_data md;
  md.verts.reserve(st->base.size());
  for (std::size_t i = 0; i + 1 < st->base.size(); i += 2) {
    md.verts.push_back(st->base[i] + st->dx);
    md.verts.push_back(st->base[i + 1] + st->dy);
  }
  outs[0] = syg::generated::make_mesh(std::move(md));
}

}  // namespace

extern const syg::crown::native_type spanv_native;
const syg::crown::native_type spanv_native{
    "spanv", [] { return static_cast<void*>(nullptr); }, [](void*) {},
    [](void*, const char*, double) {}, [](void*, const char*, const char*) {},
    no_process, nullptr,
    syg::generated::spanv_in_ports(), syg::generated::spanv_out_ports()};

extern const syg::crown::native_type mix_native;
const syg::crown::native_type mix_native{
    "mix", [] { return static_cast<void*>(new n_state()); },
    [](void* s) { delete static_cast<n_state*>(s); },
    n_set, [](void*, const char*, const char*) {}, mix_process, nullptr,
    syg::generated::mix_in_ports(), syg::generated::mix_out_ports()};

extern const syg::crown::native_type instanced_draw_native;
const syg::crown::native_type instanced_draw_native{
    "instanced_draw", [] { return static_cast<void*>(new draw_state()); },
    [](void* s) { delete static_cast<draw_state*>(s); },
    draw_set, [](void*, const char*, const char*) {}, no_process,
    draw_value_tick, syg::generated::instanced_draw_in_ports(),
    syg::generated::instanced_draw_out_ports(), false, false, nullptr,
    nullptr, draw_sapply, draw_semit};

extern const syg::crown::native_type render_head_native;
const syg::crown::native_type render_head_native{
    "render_head", [] { return static_cast<void*>(new head_state()); },
    [](void* s) { delete static_cast<head_state*>(s); },
    [](void*, const char*, double) {}, [](void*, const char*, const char*) {},
    no_process, head_value_tick,
    syg::generated::render_head_in_ports(),
    syg::generated::render_head_out_ports(), true, false, nullptr, nullptr,
    nullptr, head_semit};

// draw: the render boundary. The head-chain machinery (sapply 'tick' /
// semit 'chain') is VERBATIM instanced_draw (pkg42 stays green); the
// mesh+surface value inlets make it a structured frame node whose geometry
// the render_region executor reads via the wired producers' souts. The
// node itself issues no GL and holds no device (ADR-015, L9) — no
// svalue_tick needed; it is purely a chain node + a structural marker.
extern const syg::crown::native_type draw_native;
const syg::crown::native_type draw_native{
    "draw", [] { return static_cast<void*>(new draw_state()); },
    [](void* s) { delete static_cast<draw_state*>(s); },
    [](void*, const char*, double) {}, [](void*, const char*, const char*) {},
    no_process, nullptr, syg::generated::draw_in_ports(),
    syg::generated::draw_out_ports(), false, false, nullptr, nullptr,
    draw_sapply, draw_semit};

extern const syg::crown::native_type surface_flat_native;
const syg::crown::native_type surface_flat_native{
    "surface_flat", [] { return static_cast<void*>(new surface_flat_state()); },
    [](void* s) { delete static_cast<surface_flat_state*>(s); },
    sf_set, [](void*, const char*, const char*) {}, no_process, nullptr,
    syg::generated::surface_flat_in_ports(),
    syg::generated::surface_flat_out_ports(), false, false, nullptr,
    sf_svalue_tick, nullptr, nullptr};

extern const syg::crown::native_type mesh_from_spans_native;
const syg::crown::native_type mesh_from_spans_native{
    "mesh_from_spans", [] { return static_cast<void*>(new mesh_from_spans_state()); },
    [](void* s) { delete static_cast<mesh_from_spans_state*>(s); },
    mfs_set, mfs_set_text, no_process, nullptr,
    syg::generated::mesh_from_spans_in_ports(),
    syg::generated::mesh_from_spans_out_ports(), false, false, nullptr,
    mfs_svalue_tick, nullptr, nullptr};

}  // namespace syg::nodes
