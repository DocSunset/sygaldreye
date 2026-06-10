// Copyright 2026 Travis West
#include "host_app.hpp"
#include "eyeballs_node_abi.hpp"
#include "water_surface.hpp"
#include "sky_dome.hpp"
#include "sun_light.hpp"
#include "cube_node.hpp"
#include "lissajous.hpp"
#include "aurora.hpp"
#include "chladni.hpp"
#include "terrain_generator.hpp"
#include "particle_system.hpp"
#include "reaction_diffusion.hpp"
#include "trigger_edge.hpp"
#include <GLES3/gl3.h>

namespace {
constexpr const char* kDefaultGraph = R"({
    "nodes":[
        {"id":"sky","type":"sky_dome","params":{}},
        {"id":"water","type":"water_surface","params":{}},
        {"id":"sun","type":"sun_light","params":{}},
        {"id":"cube","type":"cube","params":{}}
    ],
    "edges":[]
})";
} // namespace

void HostApp::init(int http_port) {
    registry_.register_builtin(make_descriptor<WaterSurface>());
    registry_.register_builtin(make_descriptor<SkyDome>());
    registry_.register_builtin(make_descriptor<SunLight>());
    registry_.register_builtin(make_descriptor<CubeNode>());
    registry_.register_builtin(make_descriptor<Lissajous>());
    registry_.register_builtin(make_descriptor<Aurora>());
    registry_.register_builtin(make_descriptor<Chladni>());
    registry_.register_builtin(make_descriptor<TerrainRenderer>());
    registry_.register_builtin(make_descriptor<ParticleSystem>());
    registry_.register_builtin(make_descriptor<ReactionDiffusion>());
    registry_.register_builtin(make_descriptor<TriggerEdge>());

    active_ = parse_graph(kDefaultGraph, registry_);

    install_routes();
    http_.start(http_port,
        [](std::string_view) -> std::string { return "{}"; },
        [](std::string_view) -> std::string { return "{}"; });
}

void HostApp::frame(int width, int height, double time_s) {
    {
        std::lock_guard<std::mutex> lock(graph_mutex_);
        if (pending_) {
            active_ = std::move(pending_);
            if (editor_ready_) vr_editor_.on_graph_changed(active_.get());
        }
    }
    if (!editor_ready_) {
        text_mesh_.init();
        vr_editor_.init(registry_, active_.get());
        editor_ready_ = true;
    }

    glViewport(0, 0, width, height);
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    if (active_) {
        tick_graph(*active_, time_s);
        std::lock_guard<std::mutex> lock(values_mutex_);
        values_snapshot_ = active_->values;
    }

    // Editor update through the same inputs the Quest provides.
    VirtualControls c;
    {
        std::lock_guard<std::mutex> lock(ctrl_mutex_);
        c = ctrl_;
    }
    float dt = (prev_time_s_ > 0.0) ? float(time_s - prev_time_s_) : 0.016f;
    prev_time_s_ = time_s;
    auto edit = vr_editor_.update(&c.left, &c.right, c.trigger_left,
                                  c.trigger_right, c.grip_right,
                                  {c.thumb_x, c.thumb_y}, dt,
                                  active_.get(), registry_);
    if (edit) {
        if (auto g = parse_graph(edit->new_graph_json, registry_)) {
            std::lock_guard<std::mutex> lock(graph_mutex_);
            pending_ = std::move(g);
        }
    }

    FlyCamera cam = camera();
    float aspect = (height > 0) ? float(width) / float(height) : 1.f;
    const Eigen::Matrix4f pv = cam.proj(aspect) * cam.view();
    if (active_) {
        for (auto& call : active_->draw_calls) call(pv);
    }
    vr_editor_.draw(pv, text_mesh_);

    fulfil_screenshot(width, height);
}

FlyCamera HostApp::camera() {
    std::lock_guard<std::mutex> lock(cam_mutex_);
    return cam_;
}

void HostApp::set_camera(const FlyCamera& c) {
    std::lock_guard<std::mutex> lock(cam_mutex_);
    cam_ = c;
}
