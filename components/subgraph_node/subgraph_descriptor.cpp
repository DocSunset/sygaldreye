// Copyright 2025 Travis West
#include "subgraph_node.hpp"
#include <array>
#include <utility>

// C function pointers cannot capture, and create() takes no argument. To give
// each subgraph type a distinct create() that knows its own template, we hand
// out one of a fixed pool of template-generated trampolines, each carrying a
// compile-time slot index into a global descriptor table. A SubgraphDescriptor
// claims a slot at construction and releases it at destruction. Single-threaded
// (render thread) use only.

namespace {

constexpr int kMaxSubgraphTypes = 256;
SubgraphDescriptor* g_slots[kMaxSubgraphTypes] = {};

void* create_in_slot(int slot) {
    auto* d = g_slots[slot];
    if (!d) return nullptr;
    return new SubgraphNode(clone_graph(*d->template_ptr()));
}

template <int Slot>
void* create_trampoline() { return create_in_slot(Slot); }

template <int... Is>
constexpr auto make_trampoline_table(std::integer_sequence<int, Is...>) {
    return std::array<void* (*)(), sizeof...(Is)>{create_trampoline<Is>...};
}

const auto g_trampolines =
    make_trampoline_table(std::make_integer_sequence<int, kMaxSubgraphTypes>{});

int claim_slot(SubgraphDescriptor* d) {
    for (int i = 0; i < kMaxSubgraphTypes; ++i)
        if (!g_slots[i]) { g_slots[i] = d; return i; }
    return -1;
}

} // namespace

static std::string build_port_schema(const Graph& g) {
    std::string s = "{\"inputs\":[";
    bool first = true;
    for (const auto& d : g.inlets) {
        if (!first) s += ',';
        first = false;
        s += "{\"name\":\""; s += d.name; s += "\",\"kind\":\"unknown\"}";
    }
    s += "],\"outputs\":[";
    first = true;
    for (const auto& d : g.outlets) {
        if (!first) s += ',';
        first = false;
        s += "{\"name\":\""; s += d.name; s += "\",\"kind\":\"unknown\"}";
    }
    s += "]}";
    return s;
}

SubgraphDescriptor::SubgraphDescriptor(std::unique_ptr<Graph> graph_template,
                                       std::string type_name)
    : graph_template_(std::move(graph_template)),
      type_name_(std::move(type_name)),
      port_schema_(build_port_schema(*graph_template_)) {
    slot_ = claim_slot(this);

    desc_.version     = EYEBALLS_ABI_VERSION;
    desc_.type_name   = type_name_.c_str();
    desc_.port_schema = port_schema_.c_str();
    desc_.create      = (slot_ >= 0) ? g_trampolines[slot_] : nullptr;
    desc_.destroy = [](void* p) { delete static_cast<SubgraphNode*>(p); };
    desc_.process = [](void* p, double t) { (*static_cast<SubgraphNode*>(p))(t); };
    desc_.push_outputs = [](void* p, EyeballsOutputCtx* ctx) {
        static_cast<SubgraphNode*>(p)->push_outlets(ctx);
    };
    desc_.push_draw_calls = [](void* p, void* ctx) {
        static_cast<SubgraphNode*>(p)->push_draw_calls_to(static_cast<DrawCallCtx*>(ctx));
    };
    desc_.set_scalar_in = [](void* p, const char* port, double v) {
        static_cast<SubgraphNode*>(p)->cache_inlet(port, PortValue{v});
    };
    desc_.set_vec2_in = [](void* p, const char* port, float x, float y) {
        static_cast<SubgraphNode*>(p)->cache_inlet(port, PortValue{Eigen::Vector2f{x, y}});
    };
    desc_.set_vec3_in = [](void* p, const char* port, float x, float y, float z) {
        static_cast<SubgraphNode*>(p)->cache_inlet(port, PortValue{Eigen::Vector3f{x, y, z}});
    };
    desc_.set_vec4_in = [](void* p, const char* port, float x, float y, float z, float w) {
        static_cast<SubgraphNode*>(p)->cache_inlet(port, PortValue{Eigen::Vector4f{x, y, z, w}});
    };
    desc_.set_mat4_in = [](void* p, const char* port, const float* c16) {
        static_cast<SubgraphNode*>(p)->cache_inlet(
            port, PortValue{Eigen::Matrix4f{Eigen::Map<const Eigen::Matrix4f>(c16)}});
    };
    desc_.set_quat_in = [](void* p, const char* port, float x, float y, float z, float w) {
        static_cast<SubgraphNode*>(p)->cache_inlet(
            port, PortValue{Eigen::Quaternionf{w, x, y, z}});
    };
    desc_.set_texture_in = [](void* p, const char* port, unsigned int id, int w, int h,
                              unsigned int fmt, unsigned int filt) {
        static_cast<SubgraphNode*>(p)->cache_inlet(port, PortValue{GpuTexture{id, w, h, fmt, filt}});
    };
    desc_.set_audio_in = [](void* p, const char* port, const float* d, int f, int c, int r) {
        static_cast<SubgraphNode*>(p)->cache_inlet(port, PortValue{AudioBuffer{d, f, c, r}});
    };
    desc_.set_drawfn_in = [](void* p, const char* port, const void* fn) {
        static_cast<SubgraphNode*>(p)->cache_inlet(port, PortValue{*static_cast<const DrawFn*>(fn)});
    };
}

SubgraphDescriptor::~SubgraphDescriptor() {
    if (slot_ >= 0 && g_slots[slot_] == this) g_slots[slot_] = nullptr;
}

void SubgraphDescriptorDeleter::operator()(SubgraphDescriptor* p) const { delete p; }
