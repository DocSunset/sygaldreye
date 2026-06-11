// Copyright 2026 Travis West
#include "peer_core.hpp"
#include "editor_node.hpp"
#include "spawner_node.hpp"
#include "fly_camera_node.hpp"
#include <GLES3/gl3.h>
#include <algorithm>
#include <dirent.h>
#include <vector>

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
    http_.start(cfg_.http_port,
        [](std::string_view) -> std::string { return "{}"; },
        [](std::string_view) -> std::string { return "{}"; });
}

void PeerCore::begin_frame() {
    {
        std::lock_guard<std::mutex> lock(graph_mutex_);
        if (pending_) {
            if (active_) migrate_graph(*pending_, *active_);
            active_ = std::move(pending_);
            if (on_graph_swapped) on_graph_swapped(active_.get());
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
    for (auto& n : active_->nodes) {
        std::string_view type{n.desc->type_name};
        if (type == "editor")
            static_cast<EditorNode*>(n.data)->set_context(active_.get(), &registry,
                                                          &edit_events_);
        else if (type == "spawner")
            static_cast<SpawnerNode*>(n.data)->set_context(active_.get(), &registry,
                                                           &edit_events_);
        else if (type == "fly_camera")
            static_cast<FlyCameraNode*>(n.data)->inputs.aspect.value = aspect;
    }
}

void PeerCore::tick(double time_s) {
    if (!active_) return;
    tick_graph(*active_, time_s);
    {
        std::lock_guard<std::mutex> lock(values_mutex_);
        values_snapshot_ = active_->values;
    }
    push_remote_outs();
}

void PeerCore::collect_edits() {
    for (auto& edit : edit_events_.drain()) {
        if (auto g = parse_graph(edit, registry)) {
            std::lock_guard<std::mutex> lock(graph_mutex_);
            pending_ = std::move(g);
        }
    }
}

void PeerCore::queue_param(std::string node_id, std::string params_json) {
    param_events_.push({std::move(node_id), std::move(params_json)});
}

void PeerCore::queue_edit(std::string graph_json) {
    edit_events_.push(std::move(graph_json));
}

std::optional<PortValue> PeerCore::probe(const std::string& key) {
    std::lock_guard<std::mutex> lock(values_mutex_);
    auto it = values_snapshot_.find(key);
    if (it == values_snapshot_.end()) return std::nullopt;
    return it->second;
}

void PeerCore::fulfil_screenshot(int width, int height) {
    std::lock_guard<std::mutex> lock(shot_mutex_);
    if (shot_path_.empty() || shot_done_) return;
    std::vector<unsigned char> px(size_t(width) * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    std::vector<unsigned char> flip(px.size());
    for (int row = 0; row < height; ++row)
        std::copy_n(&px[size_t(height - 1 - row) * width * 4],
                    size_t(width) * 4, &flip[size_t(row) * width * 4]);
    shot_ok_ = stbi_write_png(shot_path_.c_str(), width, height, 4,
                              flip.data(), width * 4) != 0;
    shot_done_ = true;
    shot_cv_.notify_all();
}
