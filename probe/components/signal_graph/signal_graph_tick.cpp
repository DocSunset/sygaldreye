// Copyright 2025 Travis West
#include <cstdio>
#include <cstring>
#include <optional>

#include "signal_graph.hpp"
#include "signal_graph_plan.hpp"

// ── shared executor primitives (every scheduler uses these) ────────────────

void apply_value(const NodeInstance& n, const char* port, const PortValue& value) {
    std::visit(
        [&](const auto& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, double>) {
                if (n.desc->set_scalar_in) n.desc->set_scalar_in(n.data, port, val);
            } else if constexpr (std::is_same_v<T, Eigen::Vector2f>) {
                if (n.desc->set_vec2_in) n.desc->set_vec2_in(n.data, port, val.x(), val.y());
            } else if constexpr (std::is_same_v<T, Eigen::Vector3f>) {
                if (n.desc->set_vec3_in)
                    n.desc->set_vec3_in(n.data, port, val.x(), val.y(), val.z());
            } else if constexpr (std::is_same_v<T, Eigen::Vector4f>) {
                if (n.desc->set_vec4_in)
                    n.desc->set_vec4_in(n.data, port, val.x(), val.y(), val.z(), val.w());
            } else if constexpr (std::is_same_v<T, Eigen::Matrix4f>) {
                if (n.desc->set_mat4_in) n.desc->set_mat4_in(n.data, port, val.data());
            } else if constexpr (std::is_same_v<T, Eigen::Quaternionf>) {
                if (n.desc->set_quat_in)
                    n.desc->set_quat_in(n.data, port, val.x(), val.y(), val.z(), val.w());
            } else if constexpr (std::is_same_v<T, GpuTexture>) {
                if (n.desc->set_texture_in)
                    n.desc->set_texture_in(
                        n.data,
                        port,
                        val.id,
                        val.width,
                        val.height,
                        val.internal_format,
                        val.filter);
            } else if constexpr (std::is_same_v<T, AudioBuffer>) {
                if (n.desc->set_audio_in)
                    n.desc->set_audio_in(
                        n.data, port, val.data, val.frames, val.channels, val.sample_rate);
            } else if constexpr (std::is_same_v<T, MeshPtr>) {
                if (n.desc->set_mesh_in)
                    n.desc->set_mesh_in(n.data, port, static_cast<const void*>(&val));
            } else if constexpr (std::is_same_v<T, std::string>) {
                if (n.desc->version >= 6 && n.desc->set_text_in)
                    n.desc->set_text_in(n.data, port, val.c_str());
            }
        },
        value);
}

EyeballsOutputCtx output_ctx(
    std::unordered_map<std::string, PortValue>* store, const char* node_id) {
    using Store = std::unordered_map<std::string, PortValue>;
    EyeballsOutputCtx ctx{};
    ctx.store = store;
    ctx.node_id = node_id;
    ctx.emit_scalar = [](void* s, const char* nid, const char* port, double v) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] = v;
    };
    ctx.emit_vec2 = [](void* s, const char* nid, const char* port, float x, float y) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] = Eigen::Vector2f{x, y};
    };
    ctx.emit_vec3 = [](void* s, const char* nid, const char* port, float x, float y, float z) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] = Eigen::Vector3f{x, y, z};
    };
    ctx.emit_vec4 =
        [](void* s, const char* nid, const char* port, float x, float y, float z, float w) {
            (*static_cast<Store*>(s))[std::string(nid) + "." + port] = Eigen::Vector4f{x, y, z, w};
        };
    ctx.emit_mat4 = [](void* s, const char* nid, const char* port, const float* c16) {
        Eigen::Matrix4f mat;
        std::copy(c16, c16 + 16, mat.data());
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] = mat;
    };
    ctx.emit_quat =
        [](void* s, const char* nid, const char* port, float x, float y, float z, float w) {
            (*static_cast<Store*>(s))[std::string(nid) + "." + port] =
                Eigen::Quaternionf{w, x, y, z};
        };
    ctx.emit_texture = [](void* s,
                          const char* nid,
                          const char* port,
                          unsigned int id,
                          int w,
                          int h,
                          unsigned int fmt,
                          unsigned int filt) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] = GpuTexture{id, w, h, fmt, filt};
    };
    ctx.emit_mesh = [](void* s, const char* nid, const char* port, const void* mesh) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] =
            *static_cast<const MeshPtr*>(mesh);
    };
    ctx.emit_audio = [](void* s,
                        const char* nid,
                        const char* port,
                        const float* data,
                        int frames,
                        int channels,
                        int rate) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] =
            AudioBuffer{data, frames, channels, rate};
    };
    ctx.emit_text = [](void* s, const char* nid, const char* port, const char* utf8) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] = std::string{utf8};
    };
    ctx.emit_span = [](void* s,
                       const char* nid,
                       const char* port,
                       const float* data,
                       int rows,
                       int cols,
                       int row_axis,
                       int col_axis) {
        (*static_cast<Store*>(s))[std::string(nid) + "." + port] =
            Span{data, rows, cols, Axis(row_axis), Axis(col_axis)};
    };
    return ctx;
}

// Typed read of a producer's owned storage — the by-ref edge primitive.
// Kinds mirror detail::v6_kind; scalar outs are float by convention.
std::optional<PortValue> read_output(
    const NodeInstance& n, const std::string& port, const std::string& kind) {
    if (!n.desc->output_ptr) return std::nullopt;
    const void* p = n.desc->output_ptr(n.data, port.c_str());
    if (!p) return std::nullopt;
    if (kind == "scalar") return PortValue{double(*static_cast<const float*>(p))};
    if (kind == "bang") return PortValue{*static_cast<const bool*>(p) ? 1.0 : 0.0};
    if (kind == "bool") return PortValue{*static_cast<const bool*>(p) ? 1.0 : 0.0};
    if (kind == "vec2") return PortValue{*static_cast<const Eigen::Vector2f*>(p)};
    if (kind == "vec3") return PortValue{*static_cast<const Eigen::Vector3f*>(p)};
    if (kind == "vec4") return PortValue{*static_cast<const Eigen::Vector4f*>(p)};
    if (kind == "quat") return PortValue{*static_cast<const Eigen::Quaternionf*>(p)};
    if (kind == "mat4") return PortValue{*static_cast<const Eigen::Matrix4f*>(p)};
    if (kind == "texture") return PortValue{*static_cast<const GpuTexture*>(p)};
    if (kind == "audio") return PortValue{*static_cast<const AudioBuffer*>(p)};
    if (kind == "mesh") return PortValue{*static_cast<const MeshPtr*>(p)};
    if (kind == "mesh_array") return PortValue{*static_cast<const MeshArray*>(p)};
    if (kind == "text") return PortValue{*static_cast<const std::string*>(p)};
    if (kind == "span") return PortValue{*static_cast<const Span*>(p)};
    if (kind == "surface") return PortValue{*static_cast<const Surface*>(p)};
    if (kind == "drawable") return PortValue{*static_cast<const Mesh*>(p)};
    return std::nullopt;
}

std::optional<PortValue> read_output(const EdgeApplier& a) {
    if (!a.from) return std::nullopt;
    return read_output(*a.from, a.edge->from_port, a.kind);
}

// ── lift replay (conformability.md "lift frames") ───────────────────────────
// One implementation for every lifted host: reconcile N instances against the
// live array (keyed by cell value when the host declares a lift_key, else by
// index — so state survives a reorder), feed each row, tick, gather outputs.

namespace {

// Per-row key: from the SEPARATE key Span when one is wired (card keys on its
// stable id, not the lifted cell), else the lifted cell itself (L1), else the
// row index. Keying on a stable source is what lets a card keep its lifted
// state when the array reorders or the graph is edited live.
std::string row_key(const LiftGroup& lg, const Span& cell, const Span& key, int r) {
    if (lg.key_port.empty()) return std::to_string(r);
    const Span& s = (key.data && lg.key_port != lg.in_port) ? key : cell;
    if (!s.data) return std::to_string(r);
    std::string k;
    for (int c = 0; c < s.cols; ++c) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%g,", double(s.data[std::size_t(r) * s.cols + c]));
        k += buf;
    }
    return k;
}

// Copy a lifted instance's gathered output into the group's gather slot.
void gather_row(LiftGroup& lg, const NodeInstance& inst, int /*row*/) {
    if (lg.out_kind == "mesh") {
        auto v = read_output(inst, lg.out_port, "mesh");
        lg.mesh_gather.push_back(v ? std::get<MeshPtr>(*v) : nullptr);
        return;
    }
    auto v = read_output(inst, lg.out_port, lg.out_kind);
    auto push = [&](std::initializer_list<float> fs) {
        for (float f : fs) lg.gather_buf.push_back(f);
    };
    if (!v) {
        for (int i = 0; i < lg.cell; ++i) lg.gather_buf.push_back(0.f);
        return;
    }
    std::visit(
        [&](const auto& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, double>)
                push({float(val)});
            else if constexpr (std::is_same_v<T, Eigen::Vector2f>)
                push({val.x(), val.y()});
            else if constexpr (std::is_same_v<T, Eigen::Vector3f>)
                push({val.x(), val.y(), val.z()});
            else if constexpr (std::is_same_v<T, Eigen::Vector4f>)
                push({val.x(), val.y(), val.z(), val.w()});
            else if constexpr (std::is_same_v<T, Eigen::Quaternionf>)
                push({val.x(), val.y(), val.z(), val.w()});
            else if constexpr (std::is_same_v<T, Eigen::Matrix4f>)
                for (int i = 0; i < 16; ++i) lg.gather_buf.push_back(val.data()[i]);
        },
        *v);
}

// Bind row r's cell to the lifted input. Eigen vec/mat are contiguous
// floats, so a row pointer wires straight in.
void bind_row(NodeInstance& inst, const LiftGroup& lg, const Span& src, int r) {
    if (inst.desc->connect)
        inst.desc->connect(
            inst.data,
            lg.in_port.c_str(),
            static_cast<const void*>(src.data + std::size_t(r) * lg.cell));
}

// Bind row r's key cell to the lifted key input (so a keyed subgraph instance
// receives its id alongside its lifted cell). Only when a separate key source
// is wired (key_port != in_port).
void bind_key(NodeInstance& inst, const LiftGroup& lg, const Span& key, int r) {
    if (key.data && lg.key_port != lg.in_port && inst.desc->connect)
        inst.desc->connect(
            inst.data,
            lg.key_port.c_str(),
            static_cast<const void*>(key.data + std::size_t(r) * lg.key_cell));
}

// Clones: reconcile N stored instances against the live array (keyed by the
// separate key Span / cell value when the host declares a lift_key, else by
// index — state survives a reorder), feed each row, tick, gather.
void tick_clones(
    Graph& g, LiftGroup& lg, const Span& src, const Span& key, int rows, double time_s) {
    const NodeInstance& host = g.nodes[lg.host];
    std::string base = host.id + "." + lg.in_port + "#";
    lg.instances.clear();
    lg.instances.reserve(std::size_t(rows));
    std::vector<std::string> live;
    live.reserve(std::size_t(rows));
    for (int r = 0; r < rows; ++r) {
        std::string sk = base + row_key(lg, src, key, r);
        auto it = g.lifted_store.find(sk);
        if (it == g.lifted_store.end()) {
            LiftedInstance li{host.desc, host.desc->create()};
            // host params (and inner wiring for subgraphs) come from create();
            // copy the host's serialized params so clones match the template.
            if (host.desc->serialize && host.desc->deserialize && li.data) {
                const char* s = host.desc->serialize(host.data);
                host.desc->deserialize(li.data, s);
                if (host.desc->free_str) host.desc->free_str(s);
            }
            it = g.lifted_store.emplace(sk, li).first;
        }
        live.push_back(sk);
        lg.instances.push_back({it->second.desc, it->second.data, sk});
    }
    // Drop store entries no longer live (array shrank / keys changed).
    std::unordered_set<std::string> keep(live.begin(), live.end());
    for (auto it = g.lifted_store.begin(); it != g.lifted_store.end();) {
        bool mine = it->first.rfind(base, 0) == 0;
        if (mine && !keep.count(it->first)) {
            if (it->second.desc->destroy) it->second.desc->destroy(it->second.data);
            it = g.lifted_store.erase(it);
        } else
            ++it;
    }
    for (int r = 0; r < rows; ++r) {
        NodeInstance& inst = lg.instances[std::size_t(r)];
        bind_row(inst, lg, src, r);
        bind_key(inst, lg, key, r);
        if (inst.desc->process) inst.desc->process(inst.data, time_s);
        gather_row(lg, inst, r);
    }
}

// CellMap: a stateless host — loop the ONE host (the live node) over rows, no
// per-row state. No store, no clones (the whole point of declaring stateless).
void tick_cellmap(
    Graph& g, LiftGroup& lg, const Span& src, const Span& key, int rows, double time_s) {
    NodeInstance host = g.nodes[lg.host];  // copy: desc + data, not owned
    for (int r = 0; r < rows; ++r) {
        bind_row(host, lg, src, r);
        bind_key(host, lg, key, r);
        if (host.desc->process) host.desc->process(host.data, time_s);
        gather_row(lg, host, r);
    }
}

void tick_lift_group(Graph& g, LiftGroup& lg, double time_s) {
    Span src{};
    if (auto v = read_output(lg.source); v && std::holds_alternative<Span>(*v))
        src = std::get<Span>(*v);
    Span key{};
    if (lg.key_source.from)
        if (auto v = read_output(lg.key_source); v && std::holds_alternative<Span>(*v))
            key = std::get<Span>(*v);
    int rows = (src.data && src.cols == lg.cell) ? src.rows : 0;

    lg.gather_buf.clear();
    lg.mesh_gather.clear();
    if (lg.strategy == LiftGroup::CellMap)
        tick_cellmap(g, lg, src, key, rows, time_s);
    else
        tick_clones(g, lg, src, key, rows, time_s);
    lg.gather = Span{
        lg.gather_buf.empty() ? nullptr : lg.gather_buf.data(),
        rows,
        lg.out_cell,
        Axis::Item,
        Axis::Cell};
    // mesh out (forest route 1): N distinct meshes as a borrowed array view.
    lg.mesh_view = MeshArray{
        lg.mesh_gather.empty() ? nullptr : lg.mesh_gather.data(), int(lg.mesh_gather.size())};
}

}  // namespace

// ── the render (frame) scheduler ────────────────────────────────────────────

void tick_graph(Graph& g, double time_s) {
    if (!g.plan) {
        g.plan = build_plan(g);
        wire_plan(g);
    }
    TickPlan& plan = *g.plan;

    // Lifted hosts are replayed (N instances), not ticked in place.
    std::unordered_map<std::size_t, LiftGroup*> host_group;
    for (auto& lg : plan.lift_groups) host_group[lg.host] = &lg;

    for (std::size_t idx : plan.order) {
        auto& n = g.nodes[idx];

        if (auto hg = host_group.find(idx); hg != host_group.end()) {
            tick_lift_group(g, *hg->second, time_s);  // its source already ticked
            continue;
        }

        for (auto& a : plan.appliers[idx])
            if (auto src = read_output(a)) apply_value(n, a.edge->to_port.c_str(), *src);
        for (auto* d : plan.delayed[idx])
            if (d->prev) apply_value(n, d->applier.edge->to_port.c_str(), *d->prev);

        if (n.desc->process) n.desc->process(n.data, time_s);
    }

    // z⁻¹ capture for this region's delays (the block scheduler captures
    // its own at block end).
    for (auto& d : plan.delays)
        if (d.region == port_types::Rate::Frame)
            if (auto src = read_output(d.applier)) d.prev = *src;
}

// Pull observability: one frame-coherent sweep of every node's outputs.
// Execution never builds this — it exists only when someone asks.
std::unordered_map<std::string, PortValue> snapshot_values(const Graph& g) {
    std::unordered_map<std::string, PortValue> out;
    for (const auto& n : g.nodes)
        if (n.desc->push_outputs) {
            EyeballsOutputCtx ctx = output_ctx(&out, n.id.c_str());
            n.desc->push_outputs(n.data, &ctx);
        }
    return out;
}
