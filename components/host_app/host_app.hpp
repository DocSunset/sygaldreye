// Copyright 2026 Travis West
#pragma once
#include "component_registry.hpp"
#include "signal_graph.hpp"
#include "http_server.hpp"
#include "fly_camera.hpp"
#include "vr_editor.hpp"
#include "text_mesh.hpp"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

// Host (desktop) application spine: node registry, live graph editable over
// HTTP (POST /graph), fly camera settable over HTTP (POST /camera), and a
// frame-synchronized screenshot endpoint (POST /screenshot) for agents.
// Virtual controllers: same inputs the Quest feeds the editor, injectable
// over HTTP (POST /controller) so agents and remote humans drive the editor
// through the identical path.
struct VirtualControls {
    XrPosef left  {{0,0,0,1}, {-0.25f, 1.2f, -0.4f}};
    XrPosef right {{0,0,0,1}, { 0.25f, 1.2f, -0.4f}};
    bool  trigger_left = false, trigger_right = false, grip_right = false;
    float thumb_x = 0.f, thumb_y = 0.f;
};

struct HostApp {
    void init(int http_port);

    // Run one frame: swap pending graph, tick, draw, fulfil screenshots.
    // A GL context must be current; renders to the bound framebuffer.
    void frame(int width, int height, double time_s);

    FlyCamera camera();
    void      set_camera(const FlyCamera&);
    bool      quit_requested() const { return quit_.load(); }

private:
    void install_routes();
    void fulfil_screenshot(int width, int height);

    ComponentRegistry      registry_;
    HttpServer             http_;
    std::mutex             graph_mutex_;
    std::unique_ptr<Graph> active_;
    std::unique_ptr<Graph> pending_;

    std::mutex cam_mutex_;
    FlyCamera  cam_;

    std::mutex      ctrl_mutex_;
    VirtualControls ctrl_;
    VrEditor        vr_editor_;
    TextMesh        text_mesh_;
    bool            editor_ready_ = false;
    double          prev_time_s_  = 0.0;

    std::mutex                                 values_mutex_;
    std::unordered_map<std::string, PortValue> values_snapshot_;  // copied each tick

    std::mutex              shot_mutex_;
    std::condition_variable shot_cv_;
    std::string             shot_path_;   // non-empty: a request is pending
    bool                    shot_done_ = false;
    bool                    shot_ok_   = false;

    std::atomic_bool quit_{false};
};
