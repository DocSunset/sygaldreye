// Copyright 2026 Travis West
#include "peer_core.hpp"

#include <dirent.h>
#include <GLES3/gl3.h>

#include <algorithm>
#include <chrono>
#include <vector>

#include "card_labels_mesh.hpp"
#include "card_widgets_mesh.hpp"
#include "dwell_delete.hpp"
#include "edit_sink.hpp"
#include "editor_wires.hpp"
#include "fly_camera_node.hpp"
#include "graph_source.hpp"
#include "handle_picker.hpp"
#include "palette.hpp"
#include "palette_mesh.hpp"
#include "slider_drag.hpp"
#include "spawner_node.hpp"
#include "undo_node.hpp"
#include "wire_drag.hpp"

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
    editor_layout::GestureContext gctx{
        active_.get(), &edit_events_, &editor_overrides_, &sorted_types_};
    for (auto& n : active_->nodes) {
        std::string_view type{n.desc->type_name};
        // The meta seam: graph_source reads the live graph, edit_sink owns the
        // edit queue, the gesture nodes observe the graph + emit edit ops.
        // These replace the deleted editor monolith's set_context; spawner
        // keeps its own seam.
        if (type == "graph_source")
            static_cast<GraphSourceNode*>(n.data)->set_context(active_.get(), &editor_overrides_);
        else if (type == "edit_sink")
            static_cast<EditSinkNode*>(n.data)->set_context(&edit_events_);
        else if (type == "handle_picker")
            static_cast<HandlePickerNode*>(n.data)->set_context(gctx);
        else if (type == "wire_drag")
            static_cast<WireDragNode*>(n.data)->set_context(gctx);
        else if (type == "slider_drag")
            static_cast<SliderDragNode*>(n.data)->set_context(gctx);
        else if (type == "dwell_delete")
            static_cast<DwellDeleteNode*>(n.data)->set_context(gctx);
        else if (type == "undo_node")
            static_cast<UndoNode*>(n.data)->set_context(gctx);
        else if (type == "palette")
            static_cast<PaletteNode*>(n.data)->set_context(gctx);
        else if (type == "palette_mesh")
            static_cast<PaletteMeshNode*>(n.data)->set_context(gctx);
        else if (type == "card_widgets_mesh")
            static_cast<CardWidgetsMeshNode*>(n.data)->set_context(gctx);
        else if (type == "card_labels_mesh")
            static_cast<CardLabelsMeshNode*>(n.data)->set_context(gctx);
        else if (type == "editor_wires")
            static_cast<EditorWiresNode*>(n.data)->set_context(gctx);
        else if (type == "spawner")
            static_cast<SpawnerNode*>(n.data)->set_context(active_.get(), &registry, &edit_events_);
        else if (type == "fly_camera")
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

void PeerCore::collect_edits() {
    for (auto& edit : edit_events_.drain()) {
        // Structured ops (edit_sink) apply against the live graph; whole-graph
        // JSON (old editor) passes through apply_edit_op verbatim. Each op
        // sees the latest pending graph so a burst of ops composes.
        const Graph* base = pending_ ? pending_.get() : active_.get();
        std::string json = base ? apply_edit_op(edit, *base) : edit;
        if (auto g = parse_graph(json, registry)) {
            std::lock_guard<std::mutex> lock(graph_mutex_);
            pending_ = std::move(g);
        }
    }
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
