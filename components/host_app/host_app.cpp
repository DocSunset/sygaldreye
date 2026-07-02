// Copyright 2026 Travis West
#include "host_app.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <GLES3/gl3.h>

#include "aurora_curtain.hpp"
#include "card_labels_mesh.hpp"
#include "card_mesh.hpp"
#include "card_widgets_mesh.hpp"
#include "chladni.hpp"
#include "claude_tmux.hpp"
#include "color_mesh.hpp"
#include "cube_node.hpp"
#include "dac_node.hpp"
#include "dwell_delete.hpp"
#include "edit_sink.hpp"
#include "embedded_graphs.hpp"
#include "editor_wires.hpp"
#include "eyeballs_node_abi.hpp"
#include "flat_mesh.hpp"
#include "fly_camera.hpp"
#include "fly_camera_node.hpp"
#include "graph_source.hpp"
#include "hand_node.hpp"
#include "handle_picker.hpp"
#include "lissajous.hpp"
#include "math_nodes.hpp"
#include "mesh_nodes.hpp"
#include "net_nodes.hpp"
#include "osc_node.hpp"
#include "palette.hpp"
#include "palette_mesh.hpp"
#include "particle_system.hpp"
#include "poke_button.hpp"
#include "poke_stick.hpp"
#include "reaction_diffusion.hpp"
#include "render_region.hpp"
#include "render_region_nodes.hpp"
#include "rubber_band_controller.hpp"
#include "sample_player.hpp"
#include "sky_dome.hpp"
#include "slider_drag.hpp"
#include "spatialize_node.hpp"
#include "spawner_node.hpp"
#include "spectrogram.hpp"
#include "sprite.hpp"
#include "star_field.hpp"
#include "stb_image_write.h"
#include "stt_whisper.hpp"
#include "sun_light.hpp"
#include "terrain_generator.hpp"
#include "text_label.hpp"
#include "tree_generator_node.hpp"
#include "trigger_edge.hpp"
#include "tts_local.hpp"
#include "tts_node.hpp"
#include "ugens.hpp"
#include "ui_nodes.hpp"
#include "undo_node.hpp"
#include "vertex_color_mesh.hpp"
#include "water_surface.hpp"
#include "whisper_node.hpp"
#include "wire_drag.hpp"
#include "wire_mesh.hpp"

void register_host_nodes(ComponentRegistry& reg) {
    reg.register_builtin(make_descriptor<FlyCameraNode>());
    reg.register_builtin(make_descriptor<HandNode>());
    reg.register_builtin(make_descriptor<WireMeshNode>());
    reg.register_builtin(make_descriptor<SpawnerNode>());
    reg.register_builtin(make_descriptor<GraphSourceNode>());
    reg.register_builtin(make_descriptor<EditSinkNode>());
    reg.register_builtin(make_descriptor<CardMeshNode>());
    reg.register_builtin(make_descriptor<CardWidgetsMeshNode>());
    reg.register_builtin(make_descriptor<CardLabelsMeshNode>());
    reg.register_builtin(make_descriptor<EditorWiresNode>());
    reg.register_builtin(make_descriptor<HandlePickerNode>());
    reg.register_builtin(make_descriptor<WireDragNode>());
    reg.register_builtin(make_descriptor<SliderDragNode>());
    reg.register_builtin(make_descriptor<DwellDeleteNode>());
    reg.register_builtin(make_descriptor<UndoNode>());
    reg.register_builtin(make_descriptor<PaletteNode>());
    reg.register_builtin(make_descriptor<PaletteMeshNode>());
    reg.register_builtin(make_descriptor<LfoNode>());
    reg.register_builtin(make_descriptor<SttWhisperNode>());
    reg.register_builtin(make_descriptor<TtsLocalNode>());
    reg.register_builtin(make_descriptor<ScatterNode>());
    reg.register_builtin(make_descriptor<SeedsNode>());
    reg.register_builtin(make_descriptor<RenderHeadNode>());
    reg.register_builtin(make_descriptor<DrawNode>());
    reg.register_builtin(make_descriptor<ForestDrawNode>());
    reg.register_builtin(make_descriptor<TreeGeneratorNode>());
    reg.register_builtin(make_descriptor<ColorMeshNode>());
    reg.register_builtin(make_descriptor<VertexColorMeshNode>());
    reg.register_builtin(make_descriptor<SpriteNode>());
    reg.register_builtin(make_descriptor<PokeStickNode>());
    reg.register_builtin(make_descriptor<PokeButtonNode>());
    reg.register_builtin(make_descriptor<UiSliderNode>());
    reg.register_builtin(make_descriptor<UiButtonNode>());
    reg.register_builtin(make_descriptor<UiPaneNode>());
    reg.register_builtin(make_descriptor<RubberBandController>());
    reg.register_builtin(make_descriptor<FlatMeshNode>());
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
    reg.register_builtin(make_descriptor<Terrain>());
    reg.register_builtin(make_descriptor<ParticleSystem>());
    reg.register_builtin(make_descriptor<ReactionDiffusion>());
    reg.register_builtin(make_descriptor<MeshGridNode>());
    // MeshDisplaceNode not registered: parked, offscreen leg (owns an FBO).
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
    reg.register_builtin(make_descriptor<SpectrogramNode>());
    reg.register_builtin(make_descriptor<OscNode>());
    reg.register_builtin(make_descriptor<DacNode>());
    reg.register_builtin(make_descriptor<NoiseNode>());
    reg.register_builtin(make_descriptor<AdsrNode>());
    reg.register_builtin(make_descriptor<VcaNode>());
    reg.register_builtin(make_descriptor<MixNode>());
    reg.register_builtin(make_descriptor<BiquadNode>());
    reg.register_builtin(make_descriptor<DelayNode>());
    reg.register_builtin(make_descriptor<ShaperNode>());
    reg.register_builtin(make_descriptor<SampleHoldNode>());
    reg.register_builtin(make_descriptor<SlewNode>());
    reg.register_builtin(make_descriptor<GrainCloudNode>());
    reg.register_builtin(make_descriptor<MetroNode>());
    reg.register_builtin(make_descriptor<PercNode>());
    reg.register_builtin(make_descriptor<McPackNode>());
    reg.register_builtin(make_descriptor<McUnpackNode>());
    reg.register_builtin(make_descriptor<SpatializeNode>());
    reg.register_builtin(make_descriptor<SamplePlayerNode>());
    reg.register_builtin(make_descriptor<TtsNode>());
    reg.register_builtin(make_descriptor<WhisperNode>());

    // The card subgraph the editor lifts over the live graph (keyed by id).
    // Registered from the embedded definition so the default editor graph
    // parses identically on both shells, independent of the filesystem scan.
    reg.register_subgraph("card", embedded_graphs::kEditorCardSubgraph);
}

void HostApp::init(int http_port) {
    register_host_nodes(core_.registry);

    PeerCore::Config cfg;
    cfg.http_port = http_port;
    cfg.default_graph_json = embedded_graphs::kEditorGraph;
    cfg.data_dir = "/tmp";
    cfg.graphs_dir = "assets/graphs";
    // render_region's geometry/program caches are version/content keyed and
    // survive swaps; this drops only the graph-scoped texture entries. Runs on
    // the render thread (current GL context) inside begin_frame's swap.
    core_.on_graph_swapped = [](const Graph*) { RenderRegion::instance().notify_graph_swap(); };
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

    RenderRegion::instance().begin_frame();
    core_.tick(time_s);
    core_.collect_edits();

    Graph* g = core_.graph();
    if (!g) return;

    // The graph decides the view: camera.view/proj, if a camera node exists.
    Eigen::Matrix4f view, proj;
    auto vv = core_.read_node_output("camera", "view", "mat4");
    auto pp = core_.read_node_output("camera", "proj", "mat4");
    if (vv && pp && std::holds_alternative<Eigen::Matrix4f>(*vv) &&
        std::holds_alternative<Eigen::Matrix4f>(*pp)) {
        view = std::get<Eigen::Matrix4f>(*vv);
        proj = std::get<Eigen::Matrix4f>(*pp);
    } else {
        FlyCamera fallback{};
        proj = fallback.proj(aspect);
        view = fallback.view();
    }
    RenderRegion::instance().issue(view, proj, time_s);

    core_.fulfil_screenshot(width, height);
}
