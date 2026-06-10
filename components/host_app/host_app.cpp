// Copyright 2026 Travis West
#include "host_app.hpp"
#include "eyeballs_node_abi.hpp"
#include "fly_camera.hpp"
#include "fly_camera_node.hpp"
#include "hand_node.hpp"
#include "editor_node.hpp"
#include "math_nodes.hpp"
#include "net_nodes.hpp"
#include "ui_nodes.hpp"
#include "water_surface.hpp"
#include "sky_dome.hpp"
#include "star_field.hpp"
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
#include <dirent.h>
#include <string_view>

namespace {
// The platform graph: interaction rig wired by edges + a starter scene.
constexpr const char* kDefaultGraph = R"({
    "nodes":[
        {"id":"camera","type":"fly_camera","params":{}},
        {"id":"hand_l","type":"hand","params":{"x":-0.25}},
        {"id":"hand_r","type":"hand","params":{"x":0.25}},
        {"id":"editor","type":"editor","params":{}},
        {"id":"sky","type":"sky_dome","params":{}},
        {"id":"water","type":"water_surface","params":{}},
        {"id":"sun","type":"sun_light","params":{}},
        {"id":"cube","type":"cube","params":{}}
    ],
    "edges":[
        {"from":"hand_l.pos","to":"editor.left_pos"},
        {"from":"hand_l.rot","to":"editor.left_rot"},
        {"from":"hand_r.pos","to":"editor.right_pos"},
        {"from":"hand_r.rot","to":"editor.right_rot"},
        {"from":"hand_l.trigger","to":"editor.trigger_left"},
        {"from":"hand_r.trigger","to":"editor.trigger_right"},
        {"from":"hand_r.grip","to":"editor.grip_right"},
        {"from":"hand_l.thumb_x","to":"editor.thumb_x"},
        {"from":"hand_l.thumb_y","to":"editor.thumb_y"}
    ]
})";
} // namespace

void HostApp::init(int http_port) {
    registry_.register_builtin(make_descriptor<FlyCameraNode>());
    registry_.register_builtin(make_descriptor<HandNode>());
    registry_.register_builtin(make_descriptor<EditorNode>());
    registry_.register_builtin(make_descriptor<LfoNode>());
    registry_.register_builtin(make_descriptor<ScaleNode>());
    registry_.register_builtin(make_descriptor<AddNode>());
    registry_.register_builtin(make_descriptor<MulNode>());
    registry_.register_builtin(make_descriptor<ConstNode>());
    registry_.register_builtin(make_descriptor<SubNode>());
    registry_.register_builtin(make_descriptor<DivNode>());
    registry_.register_builtin(make_descriptor<PhasorNode>());
    registry_.register_builtin(make_descriptor<SmoothNode>());
    registry_.register_builtin(make_descriptor<UiSliderNode>());
    registry_.register_builtin(make_descriptor<UiButtonNode>());
    registry_.register_builtin(make_descriptor<UiPaneNode>());
    registry_.register_builtin(make_descriptor<UdpSendNode>());
    registry_.register_builtin(make_descriptor<UdpRecvNode>());
    registry_.register_builtin(make_descriptor<WaterSurface>());
    registry_.register_builtin(make_descriptor<SkyDome>());
    registry_.register_builtin(make_descriptor<StarField>());
    registry_.register_builtin(make_descriptor<SunLight>());
    registry_.register_builtin(make_descriptor<CubeNode>());
    registry_.register_builtin(make_descriptor<Lissajous>());
    registry_.register_builtin(make_descriptor<Aurora>());
    registry_.register_builtin(make_descriptor<Chladni>());
    registry_.register_builtin(make_descriptor<TerrainRenderer>());
    registry_.register_builtin(make_descriptor<ParticleSystem>());
    registry_.register_builtin(make_descriptor<ReactionDiffusion>());
    registry_.register_builtin(make_descriptor<TriggerEdge>());

    // Subgraph plugins: every assets/graphs/*.json becomes a node type.
    if (DIR* d = opendir("assets/graphs")) {
        while (dirent* e = readdir(d)) {
            std::string_view n{e->d_name};
            if (n.size() > 5 && n.substr(n.size() - 5) == ".json")
                registry_.load_plugin("assets/graphs/" + std::string(n));
        }
        closedir(d);
    }

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
            migrate_graph(*pending_, *active_);
            active_ = std::move(pending_);
        }
    }
    if (!active_) return;

    // Apply queued param edits (HTTP /param, windowed input pump).
    {
        std::lock_guard<std::mutex> lock(param_mutex_);
        for (auto& [id, params] : param_queue_) {
            for (auto& n : active_->nodes)
                if (n.id == id && n.desc->deserialize) {
                    n.desc->deserialize(n.data, params.c_str());
                    break;
                }
        }
        param_queue_.clear();
    }

    // Platform pumps: editor context, camera aspect.
    float aspect = (height > 0) ? float(width) / float(height) : 1.f;
    for (auto& n : active_->nodes) {
        std::string_view type{n.desc->type_name};
        if (type == "editor")
            static_cast<EditorNode*>(n.data)->set_context(active_.get(), &registry_);
        else if (type == "fly_camera")
            static_cast<FlyCameraNode*>(n.data)->inputs.aspect.value = aspect;
    }

    glViewport(0, 0, width, height);
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    tick_graph(*active_, time_s);
    {
        std::lock_guard<std::mutex> lock(values_mutex_);
        values_snapshot_ = active_->values;
    }

    // Editor edits become the next pending graph (swapped next frame, with
    // state migration — including the editor node itself).
    for (auto& n : active_->nodes) {
        if (std::string_view{n.desc->type_name} != "editor") continue;
        if (auto edit = static_cast<EditorNode*>(n.data)->take_edit()) {
            if (auto g = parse_graph(*edit, registry_)) {
                std::lock_guard<std::mutex> lock(graph_mutex_);
                pending_ = std::move(g);
            }
        }
    }

    // The graph decides the view: camera.pv, if a camera node exists.
    Eigen::Matrix4f pv;
    auto it = active_->values.find("camera.pv");
    if (it != active_->values.end() && std::holds_alternative<Eigen::Matrix4f>(it->second)) {
        pv = std::get<Eigen::Matrix4f>(it->second);
    } else {
        FlyCamera fallback{};
        pv = fallback.proj(aspect) * fallback.view();
    }
    for (auto& call : active_->draw_calls) call(pv);

    fulfil_screenshot(width, height);
}

void HostApp::queue_param(std::string node_id, std::string params_json) {
    std::lock_guard<std::mutex> lock(param_mutex_);
    param_queue_.emplace_back(std::move(node_id), std::move(params_json));
}

std::optional<PortValue> HostApp::probe(const std::string& key) {
    std::lock_guard<std::mutex> lock(values_mutex_);
    auto it = values_snapshot_.find(key);
    if (it == values_snapshot_.end()) return std::nullopt;
    return it->second;
}
