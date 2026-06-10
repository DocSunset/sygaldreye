// Copyright 2026 Travis West
#pragma once
#include "component_registry.hpp"
#include "signal_graph.hpp"
#include "http_server.hpp"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// Host (desktop) platform shell. Nearly everything lives in the graph:
// camera (fly_camera node), hands (hand nodes), the editor (editor node).
// The shell only owns the registry, the HTTP surface, the frame pump, and
// the three seams the graph cannot self-host: param injection, editor
// context, and reading camera.pv to drive the final draw.
struct HostApp {
    void init(int http_port);

    // Run one frame: swap+migrate pending graph, apply queued params, tick,
    // collect editor edits, draw, fulfil screenshots. GL context current.
    void frame(int width, int height, double time_s);

    // Thread-safe: queue {"port":value,...} params for node `id`, applied on
    // the render thread before the next tick. The HTTP /param route and the
    // windowed input pump both funnel through here.
    void queue_param(std::string node_id, std::string params_json);

    // Thread-safe read of a "node.port" value from the last tick.
    std::optional<PortValue> probe(const std::string& key);

    bool quit_requested() const { return quit_.load(); }

private:
    void install_routes();
    void fulfil_screenshot(int width, int height);

    ComponentRegistry      registry_;
    HttpServer             http_;
    std::mutex             graph_mutex_;
    std::unique_ptr<Graph> active_;
    std::unique_ptr<Graph> pending_;

    std::mutex                                       param_mutex_;
    std::vector<std::pair<std::string, std::string>> param_queue_;

    std::mutex                                 values_mutex_;
    std::unordered_map<std::string, PortValue> values_snapshot_;  // copied each tick

    std::mutex              shot_mutex_;
    std::condition_variable shot_cv_;
    std::string             shot_path_;   // non-empty: a request is pending
    bool                    shot_done_ = false;
    bool                    shot_ok_   = false;

    std::atomic_bool quit_{false};
};
