// Copyright 2026 Travis West
#include "peer_core.hpp"

#include <dirent.h>
#include <GLES3/gl3.h>

#include <algorithm>
#include <chrono>
#include <vector>

#include "fly_camera_node.hpp"

extern "C" int stbi_write_png(const char*, int, int, int, const void*, int);

void PeerCore::init(const Config& cfg) {
    cfg_ = cfg;

    if (!cfg_.graphs_dir.empty()) {
        // Subgraph plugins: every <graphs_dir>/*.json becomes a node type.
        if (DIR* d = opendir(cfg_.graphs_dir.c_str())) {
            while (dirent* e = readdir(d)) {
                std::string_view n{e->d_name};
                if (n.size() > 5 && n.substr(n.size() - 5) == ".json")
                    registry.load_plugin(cfg_.graphs_dir + "/" + std::string(n));
            }
            closedir(d);
        }
    }

    active_ = parse_graph(cfg_.default_graph_json, registry);

    install_routes();
    http_.ws_handler = [this](unsigned long conn_id, std::string_view msg) {
        return handle_ws(conn_id, msg);
    };
    http_.start(
        cfg_.http_port,
        [](std::string_view) -> std::string { return "{}"; },
        [](std::string_view) -> std::string { return "{}"; });
}

void PeerCore::begin_frame() {
    {
        std::lock_guard<std::mutex> lock(graph_mutex_);
        if (pending_) {
            // Swap, migrate, retire the old graph, and re-derive the block
            // region all while the audio callback is held off — it keeps
            // raw node pointers across a block.
            auto audio_guard = audio_.pause_blocks();
            if (active_) migrate_graph(*pending_, *active_);
            auto old = std::move(active_);
            active_ = std::move(pending_);
            audio_.rebuild_unlocked(*active_);
            ++graph_gen_;  // layout/undo caches key on this
            if (on_graph_swapped) on_graph_swapped(active_.get());
            // `old` (and every displaced instance) destructs here, inside
            // the guard.
        }
    }
    if (!active_) return;
    for (auto& [id, params] : param_events_.drain()) {
        for (auto& n : active_->nodes)
            if (n.id == id && n.desc->deserialize) {
                n.desc->deserialize(n.data, params.c_str());
                ++graph_gen_;  // in-place param write: same cache key
                break;
            }
    }
    apply_remote_sets();
}

void PeerCore::pump_contexts(float aspect) {
    if (!active_) return;
    // The sorted registry type list backs the palette; cache it (cheap, but
    // re-sort only when registration count changes — types are added by net
    // peer connects, rare).
    if (sorted_types_.size() != registry.type_names().size()) {
        sorted_types_ = registry.type_names();
        std::sort(sorted_types_.begin(), sorted_types_.end());
    }
    // The meta seam, generic (ABI v9): build the editor context once and
    // offer it to every node that declares set_host_context — gesture nodes,
    // meshes, graph_source/edit_sink/spawner, plugins, subgraph inners alike.
    // No type dispatch: extending the editor never touches this file again.
    editor_layout::GestureContext gctx{
        active_.get(),      &edit_events_,  &editor_overrides_, &sorted_types_,
        &registry,          &layout_cache_, graph_gen_,         &overrides_gen_};
    for (auto& n : active_->nodes) {
        if (n.desc->set_host_context)
            n.desc->set_host_context(n.data, editor_layout::kEditorContextKind, &gctx);
        // Value fallback, not a context seam: the camera's aspect default.
        if (std::string_view{n.desc->type_name} == "fly_camera")
            static_cast<FlyCameraNode*>(n.data)->endpoints.aspect.fallback = aspect;
    }
}

void PeerCore::tick(double time_s) {
    if (!active_) return;
    // New graph object → (re)derive the block region before its first
    // render plan is built, so block nodes never tick render-side.
    if (!active_->plan) audio_.rebuild(*active_);
    double dt = (prev_tick_t_ > 0.0) ? time_s - prev_tick_t_ : 1.0 / 60.0;
    prev_tick_t_ = time_s;

    audio_.publish(*active_);  // rings + snapshots → values
    tick_graph(*active_, time_s);
    audio_.capture_latches(*active_);  // frame control → block
    audio_.pump_offline(dt);           // no device? blocks run here
    bool hosted_active;
    {
        std::lock_guard<std::mutex> lock(hosted_mutex_);
        hosted_active = !hosted_.empty();
    }
    {
        // Pull observability: build a frame-coherent snapshot only when an
        // observer asked since the last one. Zero cost unobserved. Hosted
        // bridge nodes are STANDING observers (their outputs mirror to the
        // remote peer every tick).
        std::unique_lock<std::mutex> lock(values_mutex_);
        if (values_requested_ || hosted_active) {
            values_snapshot_ = snapshot_values(*active_);
            values_requested_ = false;
            ++values_gen_;
            values_cv_.notify_all();
        }
    }
    push_remote_outs();
}

namespace {
// {"op":"set_param","id":…,"port":…,"value":N} → the in-place fast path.
bool parse_set_param(const std::string& op, std::string& id, std::string& port, std::string& num) {
    if (op.find("\"op\":\"set_param\"") == std::string::npos) return false;
    auto field = [&](const char* key) {
        std::string out;
        std::string needle = std::string("\"") + key + "\":\"";
        auto p = op.find(needle);
        if (p == std::string::npos) return out;
        for (p += needle.size(); p < op.size() && op[p] != '"'; ++p) {
            if (op[p] == '\\' && p + 1 < op.size()) ++p;
            out += op[p];
        }
        return out;
    };
    id = field("id");
    port = field("port");
    auto vp = op.find("\"value\":");
    if (id.empty() || port.empty() || vp == std::string::npos) return false;
    auto vs = vp + 8;
    auto ve = op.find_first_of(",}", vs);
    num = op.substr(vs, (ve == std::string::npos ? op.size() : ve) - vs);
    return !num.empty();
}
}  // namespace

void PeerCore::collect_edits() {
    // set_param ops bypass the rebuild: they deserialize in place at the next
    // begin_frame via queue_param — no parse, no swap, no cache wipe; the
    // param persists because desc->serialize reads node data. A slider drag
    // then costs one small deserialize per frame instead of a whole-graph
    // rebuild at 72 Hz. Bursts coalesce to the last op per node+port.
    struct P {
        std::string id, port, num;
    };
    std::vector<P> params;
    for (auto& edit : edit_events_.drain()) {
        if (P p; parse_set_param(edit, p.id, p.port, p.num)) {
            auto it = std::find_if(params.begin(), params.end(), [&](const P& q) {
                return q.id == p.id && q.port == p.port;
            });
            if (it != params.end())
                it->num = std::move(p.num);
            else
                params.push_back(std::move(p));
            continue;
        }
        // Structural ops (edit_sink) apply against the live graph; whole-graph
        // JSON (old editor) passes through apply_edit_op verbatim. Each op
        // sees the latest pending graph so a burst of ops composes.
        const Graph* base = pending_ ? pending_.get() : active_.get();
        std::string json = base ? apply_edit_op(edit, *base) : edit;
        if (auto g = parse_graph(json, registry)) {
            std::lock_guard<std::mutex> lock(graph_mutex_);
            pending_ = std::move(g);
        }
    }
    for (auto& p : params)
        queue_param(p.id, "{\"" + editor_layout::json_escape(p.port) + "\":" + p.num + "}");
}

void PeerCore::queue_param(std::string node_id, std::string params_json) {
    param_events_.push({std::move(node_id), std::move(params_json)});
}

void PeerCore::queue_edit(std::string graph_json) { edit_events_.push(std::move(graph_json)); }

std::optional<PortValue> PeerCore::probe(const std::string& key) {
    std::unique_lock<std::mutex> lock(values_mutex_);
    std::uint64_t want = values_gen_ + 1;
    values_requested_ = true;
    values_cv_.wait_for(lock, std::chrono::milliseconds(250), [&] { return values_gen_ >= want; });
    auto it = values_snapshot_.find(key);
    if (it == values_snapshot_.end()) return std::nullopt;
    return it->second;
}

std::optional<PortValue> PeerCore::read_node_output(
    const std::string& node_id, const std::string& port, const std::string& kind) {
    std::lock_guard<std::mutex> lock(graph_mutex_);
    if (!active_) return std::nullopt;
    for (const auto& n : active_->nodes)
        if (n.id == node_id) return read_output(n, port, kind);
    return std::nullopt;
}

void PeerCore::fulfil_screenshot(int width, int height) {
    std::lock_guard<std::mutex> lock(shot_mutex_);
    if (shot_path_.empty() || shot_done_) return;
    std::vector<unsigned char> px(size_t(width) * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    std::vector<unsigned char> flip(px.size());
    for (int row = 0; row < height; ++row)
        std::copy_n(
            &px[size_t(height - 1 - row) * width * 4],
            size_t(width) * 4,
            &flip[size_t(row) * width * 4]);
    shot_ok_ = stbi_write_png(shot_path_.c_str(), width, height, 4, flip.data(), width * 4) != 0;
    shot_done_ = true;
    shot_cv_.notify_all();
}
