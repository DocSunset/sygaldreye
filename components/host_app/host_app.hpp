// Copyright 2026 Travis West
#pragma once
#include "peer_core.hpp"
#include <optional>
#include <string>

// Desktop shell over PeerCore: registers the host node vocabulary, pumps
// frames from GLFW/headless EGL, and draws via camera.pv. Everything else
// (HTTP surface, graph swap, queues, screenshots) is the portable core.
struct HostApp {
    void init(int http_port);

    // Run one frame: core phases, then draw. GL context current.
    void frame(int width, int height, double time_s);

    void queue_param(std::string node_id, std::string params_json) {
        core_.queue_param(std::move(node_id), std::move(params_json));
    }
    std::optional<PortValue> probe(const std::string& key) {
        return core_.probe(key);
    }
    bool quit_requested() const { return core_.quit_requested(); }

private:
    PeerCore core_;
};
