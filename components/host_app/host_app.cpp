// Copyright 2026 Travis West
#include "host_app.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "eyeballs_node_abi.hpp"
#include "fly_camera.hpp"
#include "fly_camera_node.hpp"
#include "hand_node.hpp"
#include "editor_node.hpp"
#include "spawner_node.hpp"
#include "math_nodes.hpp"
#include "net_nodes.hpp"
#include "ui_nodes.hpp"
#include "water_surface.hpp"
#include "sky_dome.hpp"
#include "star_field.hpp"
#include "sun_light.hpp"
#include "cube_node.hpp"
#include "lissajous.hpp"
#include "aurora_curtain.hpp"
#include "chladni.hpp"
#include "terrain_generator.hpp"
#include "particle_system.hpp"
#include "reaction_diffusion.hpp"
#include "rd_gpu.hpp"
#include "rd_renderer.hpp"
#include "render_nodes.hpp"
#include "glsl_effect.hpp"
#include "mesh_nodes.hpp"
#include "claude_tmux.hpp"
#include "trigger_edge.hpp"
#include "text_label.hpp"
#include "poke_stick.hpp"
#include "poke_button.hpp"
#include "spectrogram.hpp"
#include "osc_node.hpp"
#include "dac_node.hpp"
#include "tts_node.hpp"
#include "whisper_node.hpp"
#include <GLES3/gl3.h>

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
        {"id":"cube","type":"cube","params":{}},
        {"id":"stt","type":"whisper_stt","params":{"target_node":"claude"}},
        {"id":"claude","type":"claude_tmux","params":{}},
        {"id":"speak","type":"tts","params":{"play_url":"http://192.168.0.18:8080/play"}}
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
    auto& reg = core_.registry;
    reg.register_builtin(make_descriptor<FlyCameraNode>());
    reg.register_builtin(make_descriptor<HandNode>());
    reg.register_builtin(make_descriptor<EditorNode>());
    reg.register_builtin(make_descriptor<SpawnerNode>());
    reg.register_builtin(make_descriptor<LfoNode>());
    reg.register_builtin(make_descriptor<ScaleNode>());
    reg.register_builtin(make_descriptor<AddNode>());
    reg.register_builtin(make_descriptor<MulNode>());
    reg.register_builtin(make_descriptor<ConstNode>());
    reg.register_builtin(make_descriptor<SubNode>());
    reg.register_builtin(make_descriptor<DivNode>());
    reg.register_builtin(make_descriptor<PhasorNode>());
    reg.register_builtin(make_descriptor<SmoothNode>());
    reg.register_builtin(make_descriptor<Split3Node>());
    reg.register_builtin(make_descriptor<Join3Node>());
    reg.register_builtin(make_descriptor<HsvColorNode>());
    reg.register_builtin(make_descriptor<TimeNode>());
    reg.register_builtin(make_descriptor<ClaudeTmuxNode>());
    reg.register_builtin(make_descriptor<UiSliderNode>());
    reg.register_builtin(make_descriptor<UiButtonNode>());
    reg.register_builtin(make_descriptor<UiPaneNode>());
    reg.register_builtin(make_descriptor<UdpSendNode>());
    reg.register_builtin(make_descriptor<UdpRecvNode>());
    reg.register_builtin(make_descriptor<WaterSurface>());
    reg.register_builtin(make_descriptor<SkyDome>());
    reg.register_builtin(make_descriptor<StarField>());
    reg.register_builtin(make_descriptor<SunLight>());
    reg.register_builtin(make_descriptor<CubeNode>());
    reg.register_builtin(make_descriptor<Lissajous>());
    reg.register_builtin(make_descriptor<AuroraCurtainNode>());
    reg.register_builtin(make_descriptor<Chladni>());
    reg.register_builtin(make_descriptor<TerrainRenderer>());
    reg.register_builtin(make_descriptor<ParticleSystem>());
    reg.register_builtin(make_descriptor<ReactionDiffusion>());
    reg.register_builtin(make_descriptor<RdGpu>());
    reg.register_builtin(make_descriptor<RDRenderer>());
    reg.register_builtin(make_descriptor<RenderTargetNode>());
    reg.register_builtin(make_descriptor<TextureViewNode>());
    reg.register_builtin(make_descriptor<GlslEffectNode>());
    reg.register_builtin(make_descriptor<MeshGridNode>());
    reg.register_builtin(make_descriptor<MeshDisplaceNode>());
    reg.register_builtin(make_descriptor<MeshRenderNode>());
    reg.register_builtin(make_descriptor<MeshSphereNode>());
    reg.register_builtin(make_descriptor<MeshBoxNode>());
    reg.register_builtin(make_descriptor<MeshCylinderNode>());
    reg.register_builtin(make_descriptor<MeshRippleNode>());
    reg.register_builtin(make_descriptor<MeshTwistNode>());
    reg.register_builtin(make_descriptor<MeshTransformNode>());
    reg.register_builtin(make_descriptor<Vec3AddNode>());
    reg.register_builtin(make_descriptor<Vec3ScaleNode>());
    reg.register_builtin(make_descriptor<Vec3LerpNode>());
    reg.register_builtin(make_descriptor<Vec3DotNode>());
    reg.register_builtin(make_descriptor<Vec3CrossNode>());
    reg.register_builtin(make_descriptor<Vec3LengthNode>());
    reg.register_builtin(make_descriptor<QuatEulerNode>());
    reg.register_builtin(make_descriptor<QuatMulNode>());
    reg.register_builtin(make_descriptor<QuatRotateNode>());
    reg.register_builtin(make_descriptor<QuatSlerpNode>());
    reg.register_builtin(make_descriptor<TrsNode>());
    reg.register_builtin(make_descriptor<MatMulNode>());
    reg.register_builtin(make_descriptor<TriggerEdge>());
    reg.register_builtin(make_descriptor<TextLabelNode>());
    reg.register_builtin(make_descriptor<PokeStickNode>());
    reg.register_builtin(make_descriptor<PokeButtonNode>());
    reg.register_builtin(make_descriptor<SpectrogramNode>());
    reg.register_builtin(make_descriptor<OscNode>());
    reg.register_builtin(make_descriptor<DacNode>());
    reg.register_builtin(make_descriptor<TtsNode>());
    reg.register_builtin(make_descriptor<WhisperNode>());

    PeerCore::Config cfg;
    cfg.http_port          = http_port;
    cfg.default_graph_json = kDefaultGraph;
    cfg.data_dir           = "/tmp";
    cfg.graphs_dir         = "assets/graphs";
    core_.init(cfg);
}

void HostApp::frame(int width, int height, double time_s) {
    core_.begin_frame();
    float aspect = (height > 0) ? float(width) / float(height) : 1.f;
    core_.pump_contexts(aspect);

    glViewport(0, 0, width, height);
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    core_.tick(time_s);
    core_.collect_edits();

    Graph* g = core_.graph();
    if (!g) return;

    // The graph decides the view: camera.pv, if a camera node exists.
    Eigen::Matrix4f pv;
    auto it = g->values.find("camera.pv");
    if (it != g->values.end() && std::holds_alternative<Eigen::Matrix4f>(it->second)) {
        pv = std::get<Eigen::Matrix4f>(it->second);
    } else {
        FlyCamera fallback{};
        pv = fallback.proj(aspect) * fallback.view();
    }
    for (auto& call : g->draw_calls) call(pv);

    core_.fulfil_screenshot(width, height);
}
