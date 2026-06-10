#include <android/log.h>
#include <android_native_app_glue.h>
#include <openxr/openxr.h>
#include "renderer.hpp"
#include "xr_session.hpp"
#include "frame_loop.hpp"
#include "scene.hpp"
#include "input.hpp"
#include "cube_node.hpp"
#include "text_mesh.hpp"
#include "http_server.hpp"
#include "mdns_advertiser.hpp"
#include "param_registry.hpp"
#include "eyeballs_node_abi.hpp"
#include "speech_to_text.hpp"
#include "component_registry.hpp"
#include "signal_graph.hpp"
#include "vr_editor.hpp"
#include "rd_gpu.hpp"
#include "xr_sources.hpp"
#include "renderer_node.hpp"
#include "atmos_synth.hpp"
#include "rain_synth.hpp"
#include "fire_synth.hpp"
#include "engine_synth.hpp"
#include "water_synth.hpp"
#include "creature_synth.hpp"
#include "chime_synth.hpp"
#include "lissajous.hpp"
#include "aurora.hpp"
#include "chladni.hpp"
#include "terrain_generator.hpp"
#include "particle_system.hpp"
#include "reaction_diffusion.hpp"
#include "water_surface.hpp"
#include "sky_dome.hpp"
#include "star_field.hpp"
#include "sun_light.hpp"
#include "trigger_edge.hpp"
#include "ptt_gate.hpp"
#include "mic_input.hpp"
#include "text_label.hpp"
#include "math_nodes.hpp"
#include "net_nodes.hpp"
#include "ui_nodes.hpp"
#include "hand_node.hpp"
#include "fly_camera_node.hpp"
#include "spawner_node.hpp"
#include "aurora_curtain.hpp"
#include <GLES3/gl3.h>
#include <cstdio>
#include <ctime>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#define LOG(...)  __android_log_print(ANDROID_LOG_INFO,  "eyeballs", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  "eyeballs", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "eyeballs", __VA_ARGS__)

XrInstance xr_create_instance(struct android_app*);
XrSystemId xr_get_system(XrInstance);

struct AppState {
    bool hasWindow = false;
    bool hasFocus  = false;
    bool renderable() const { return hasWindow && hasFocus; }
    XrInstance xrInstance = XR_NULL_HANDLE;
    XrSystemId xrSystemId = XR_NULL_SYSTEM_ID;
    Renderer renderer{};
    XrSessionObj xrSession{};
    FrameLoop    frame_loop_{};
    Scene scene_{};
    Input input_{};
    TextMesh text_mesh_{};
    ComponentRegistry            registry_{};
    VrEditor                     vr_editor_{};
    std::mutex                   graph_mutex_{};
    std::unique_ptr<Graph>       pending_graph_{};
    std::unique_ptr<Graph>       active_graph_{};
    std::string                  meta_graph_json_{};
    double                       prev_frame_time_s_ = 0.0;
};

static void onAppCmd(struct android_app* app, int32_t cmd) {
    auto* s = static_cast<AppState*>(app->userData);
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:  s->hasWindow = true;  LOG("INIT_WINDOW");   break;
        case APP_CMD_TERM_WINDOW:  s->hasWindow = false; LOG("TERM_WINDOW");   break;
        case APP_CMD_GAINED_FOCUS: s->hasFocus  = true;  LOG("GAINED_FOCUS");  break;
        case APP_CMD_LOST_FOCUS:   s->hasFocus  = false; LOG("LOST_FOCUS");    break;
        case APP_CMD_DESTROY:                            LOG("DESTROY");       break;
    }
}

static void acquire_multicast_lock(struct android_app* app) {
    JNIEnv* env = nullptr;
    app->activity->vm->AttachCurrentThread(&env, nullptr);
    jobject activity  = app->activity->clazz;
    jclass  actClass  = env->GetObjectClass(activity);
    jmethodID getSvc  = env->GetMethodID(actClass, "getSystemService",
                                          "(Ljava/lang/String;)Ljava/lang/Object;");
    jstring wifiStr   = env->NewStringUTF("wifi");
    jobject wifiMgr   = env->CallObjectMethod(activity, getSvc, wifiStr);
    jclass  wifiClass = env->GetObjectClass(wifiMgr);
    jmethodID mkLock  = env->GetMethodID(wifiClass, "createMulticastLock",
                                          "(Ljava/lang/String;)Landroid/net/wifi/WifiManager$MulticastLock;");
    jstring lockTag   = env->NewStringUTF("eyeballs_mdns");
    jobject lock      = env->CallObjectMethod(wifiMgr, mkLock, lockTag);
    jclass  lockClass = env->GetObjectClass(lock);
    jmethodID acquire = env->GetMethodID(lockClass, "acquire", "()V");
    env->CallVoidMethod(lock, acquire);
    env->DeleteLocalRef(wifiStr);
    env->DeleteLocalRef(lockTag);
    app->activity->vm->DetachCurrentThread();
}

// Pump XR source nodes with current frame's live pose/input data.
static void pump_xr_sources(AppState& state, double /*time_sec*/) {
    if (!state.active_graph_) return;
    auto lh = state.input_.hand_pose(Hand::LEFT);
    auto rh = state.input_.hand_pose(Hand::RIGHT);
    bool trigger_left  = state.input_.trigger_pressed(Hand::LEFT);
    bool trigger_right = state.input_.trigger_pressed(Hand::RIGHT);

    for (auto& n : state.active_graph_->nodes) {
        std::string_view type{n.desc->type_name};
        if (type == "head_pose") {
            // HMD pose is available per-eye during render; use a zero pose here as proxy.
            // The renderer reads xrLocateViews directly, so head_pose outputs are
            // for downstream graph nodes (scene placement etc.).
            XrPosef identity{};
            identity.orientation.w = 1.0f;
            static_cast<HeadPoseNode*>(n.data)->set_pose(identity);
        } else if (type == "left_controller") {
            XrPosef p = lh ? lh->pose : XrPosef{{0,0,0,1},{0,0,0}};
            static_cast<LeftControllerNode*>(n.data)->set_state(
                p, trigger_left);
        } else if (type == "right_controller") {
            XrPosef p = rh ? rh->pose : XrPosef{{0,0,0,1},{0,0,0}};
            static_cast<RightControllerNode*>(n.data)->set_state(
                p, trigger_right);
        }
    }
}

void android_main(struct android_app* app) {
    LOG("android_main: started");
    acquire_multicast_lock(app);

    AppState state;
    state.xrInstance = xr_create_instance(app);
    state.xrSystemId = xr_get_system(state.xrInstance);
    auto binding = state.renderer.init();
    if (!binding) { LOGE("renderer init failed"); return; }
    state.xrSession.create(state.xrInstance, state.xrSystemId, &binding->xr_binding);
    state.renderer.create_swapchains(state.xrInstance, state.xrSystemId, state.xrSession.get());
    state.text_mesh_.init();
    state.input_.create(state.xrInstance, state.xrSession.get());

    state.registry_.register_builtin(make_descriptor<WaterSurface>());
    state.registry_.register_builtin(make_descriptor<SkyDome>());
    state.registry_.register_builtin(make_descriptor<StarField>());
    state.registry_.register_builtin(make_descriptor<RdGpu>());
    state.registry_.register_builtin(make_descriptor<HeadPoseNode>());
    state.registry_.register_builtin(make_descriptor<LeftControllerNode>());
    state.registry_.register_builtin(make_descriptor<RightControllerNode>());
    state.registry_.register_builtin(make_descriptor<AtmosSynth>());
    state.registry_.register_builtin(make_descriptor<RainSynth>());
    state.registry_.register_builtin(make_descriptor<FireSynth>());
    state.registry_.register_builtin(make_descriptor<EngineSynth>());
    state.registry_.register_builtin(make_descriptor<WaterSynth>());
    state.registry_.register_builtin(make_descriptor<CreatureSynth>());
    state.registry_.register_builtin(make_descriptor<ChimeSynth>());
    state.registry_.register_builtin(make_descriptor<Lissajous>());
    state.registry_.register_builtin(make_descriptor<Aurora>());
    state.registry_.register_builtin(make_descriptor<Chladni>());
    state.registry_.register_builtin(make_descriptor<TerrainRenderer>());
    state.registry_.register_builtin(make_descriptor<ParticleSystem>());
    state.registry_.register_builtin(make_descriptor<ReactionDiffusion>());
    state.registry_.register_builtin(make_descriptor<RendererNode>());
    state.registry_.register_builtin(make_descriptor<SunLight>());
    state.registry_.register_builtin(make_descriptor<TriggerEdge>());
    state.registry_.register_builtin(make_descriptor<PttGate>());
    state.registry_.register_builtin(make_descriptor<MicInputNode>());
    state.registry_.register_builtin(make_descriptor<CubeNode>());
    state.registry_.register_builtin(make_descriptor<TextLabelNode>());
    state.registry_.register_builtin(make_descriptor<SpeechToTextNode>());
    state.registry_.register_builtin(make_descriptor<LfoNode>());
    state.registry_.register_builtin(make_descriptor<ScaleNode>());
    state.registry_.register_builtin(make_descriptor<AddNode>());
    state.registry_.register_builtin(make_descriptor<MulNode>());
    state.registry_.register_builtin(make_descriptor<ConstNode>());
    state.registry_.register_builtin(make_descriptor<SubNode>());
    state.registry_.register_builtin(make_descriptor<DivNode>());
    state.registry_.register_builtin(make_descriptor<PhasorNode>());
    state.registry_.register_builtin(make_descriptor<SmoothNode>());
    state.registry_.register_builtin(make_descriptor<Split3Node>());
    state.registry_.register_builtin(make_descriptor<Join3Node>());
    state.registry_.register_builtin(make_descriptor<UdpSendNode>());
    state.registry_.register_builtin(make_descriptor<UdpRecvNode>());
    state.registry_.register_builtin(make_descriptor<UiSliderNode>());
    state.registry_.register_builtin(make_descriptor<UiButtonNode>());
    state.registry_.register_builtin(make_descriptor<UiPaneNode>());
    state.registry_.register_builtin(make_descriptor<HandNode>());
    state.registry_.register_builtin(make_descriptor<FlyCameraNode>());
    state.registry_.register_builtin(make_descriptor<SpawnerNode>());
    state.registry_.register_builtin(make_descriptor<AuroraCurtainNode>());

    constexpr const char* kDefaultGraph = R"({
        "nodes":[
            {"id":"sky","type":"sky_dome","params":{}},
            {"id":"water","type":"water_surface","params":{}},
            {"id":"head","type":"head_pose","params":{}},
            {"id":"renderer","type":"renderer","params":{}},
            {"id":"sun","type":"sun_light","params":{}},
            {"id":"cube","type":"cube","params":{}}
        ],
        "edges":[
            {"from":"sky.sun_elevation_out","to":"water.sun intensity"}
        ]
    })";
    if (auto g = parse_graph(kDefaultGraph, state.registry_))
        state.active_graph_ = std::move(g);

    state.vr_editor_.init(state.registry_, state.active_graph_.get());

    const char* data_path = app->activity->internalDataPath;
    HttpServer http_server;
    http_server.add_route("GET", "/plugins",
        [&state](std::string_view) -> std::string {
            std::string out = "[";
            bool first = true;
            for (const auto& n : state.registry_.type_names()) {
                if (!first) out += ',';
                first = false;
                out += '"'; out += n; out += '"';
            }
            out += ']';
            return out;
        });
    http_server.add_route("POST", "/plugins",
        [&state, data_path](std::string_view body) -> std::string {
            char path[256];
            std::snprintf(path, sizeof(path), "%s/%ld.so", data_path,
                          static_cast<long>(std::time(nullptr)));
            FILE* f = std::fopen(path, "wb");
            if (!f) return R"({"ok":false,"error":"fopen failed"})";
            std::fwrite(body.data(), 1, body.size(), f);
            std::fclose(f);
            if (!state.registry_.load_plugin(path))
                return R"({"ok":false,"error":"load_plugin failed"})";
            return R"({"ok":true})";
        });
    http_server.add_route("GET", "/graph",
        [&state](std::string_view) -> std::string {
            std::lock_guard<std::mutex> lock(state.graph_mutex_);
            if (state.active_graph_) return serialize_graph(*state.active_graph_);
            return "{\"nodes\":[],\"edges\":[]}";
        });
    http_server.add_route("POST", "/graph",
        [&state, &http_server](std::string_view body) -> std::string {
            auto g = parse_graph(std::string(body), state.registry_);
            if (!g) return R"({"ok":false,"error":"parse_graph failed"})";
            auto graph_json = serialize_graph(*g);
            {
                std::lock_guard<std::mutex> lock(state.graph_mutex_);
                state.pending_graph_ = std::move(g);
            }
            // Notify SSE clients of the new graph
            http_server.broadcast_event("graph", graph_json);
            return R"({"ok":true})";
        });
    http_server.add_route("GET", "/palette",
        [&state](std::string_view) -> std::string {
            std::string out = "[";
            bool first = true;
            for (const auto& type_name : state.registry_.type_names()) {
                const auto* desc = state.registry_.find(type_name);
                if (!desc) continue;
                if (!first) out += ',';
                first = false;
                out += "{\"type\":\""; out += type_name; out += "\"";
                if (desc->create && desc->serialize) {
                    void* tmp = desc->create();
                    const char* s = desc->serialize(tmp);
                    out += ",\"defaults\":"; out += s;
                    if (desc->free_str) desc->free_str(s);
                    desc->destroy(tmp);
                }
                out += '}';
            }
            out += ']';
            return out;
        });
    http_server.add_route("GET", "/meta-graph",
        [&state](std::string_view) -> std::string {
            return state.meta_graph_json_.empty() ? "{}" : state.meta_graph_json_;
        });
    http_server.add_route("POST", "/meta-graph",
        [&state](std::string_view body) -> std::string {
            state.meta_graph_json_ = std::string(body);
            return R"({"ok":true})";
        });
    http_server.start(8080,
        [](std::string_view) -> std::string { return "{}"; },
        [](std::string_view) -> std::string { return "{}"; });
    MdnsAdvertiser mdns_advertiser;
    mdns_advertiser.start("eyeballs", 8080);

    app->userData  = &state;
    app->onAppCmd  = onAppCmd;

    while (!app->destroyRequested && !state.xrSession.should_quit()) {
        int         events;
        android_poll_source* source;
        int timeout = state.xrSession.should_render() ? 0 : 10;
        if (ALooper_pollOnce(timeout, nullptr, &events,
                             reinterpret_cast<void**>(&source)) >= 0) {
            if (source) { source->process(app, source); }
        }
        if (!state.xrSession.poll_events()) {
            LOGE("poll_events failed — exiting loop");
            break;
        }
        if (state.xrSession.session_running()) {
            state.frame_loop_.run_frame(state.xrSession.get(), [&](XrTime t) -> FrameLayers {
                double time_sec = static_cast<double>(t) * 1e-9;

                // Atomic swap of pending graph at frame boundary
                {
                    std::lock_guard<std::mutex> lock(state.graph_mutex_);
                    if (state.pending_graph_) {
                        migrate_graph(*state.pending_graph_, *state.active_graph_);
                        state.active_graph_ = std::move(state.pending_graph_);
                        state.vr_editor_.on_graph_changed(state.active_graph_.get());
                    }
                }

                if (!state.input_.sync(state.xrSession.get(), state.xrSession.worldSpace_(), t,
                                       state.xrSession.should_render())) {
                    LOGW("input sync failed — skipping input");
                }

                auto lh = state.input_.hand_pose(Hand::LEFT);
                auto rh = state.input_.hand_pose(Hand::RIGHT);

                // Pump XR source nodes with live pose/input data
                pump_xr_sources(state, time_sec);

                // Tick the graph: topo-sort, propagate values, collect draw calls
                if (state.active_graph_) tick_graph(*state.active_graph_, time_sec);

                // VR editor: palette, node cards, drag-to-connect, sliders, delete/undo
                // grip_right → wire drag; thumbstick_left → undo; delta_time_s → dwell
                // GraphEdit → parse and atomic-swap to pending_graph_
                {
                    const XrPosef* lp = lh ? &lh->pose : nullptr;
                    const XrPosef* rp = rh ? &rh->pose : nullptr;
                    bool tl = state.input_.trigger_pressed(Hand::LEFT);
                    bool tr = state.input_.trigger_pressed(Hand::RIGHT);
                    bool gr = state.input_.grip_pressed(Hand::RIGHT);
                    Eigen::Vector2f ts = state.input_.thumbstick(Hand::LEFT);
                    float dt = (state.prev_frame_time_s_ > 0.0)
                        ? static_cast<float>(time_sec - state.prev_frame_time_s_)
                        : 0.016f;
                    auto edit = state.vr_editor_.update(
                        lp, rp, tl, tr, gr, ts, dt,
                        state.active_graph_.get(), state.registry_);
                    if (edit) {
                        auto g = parse_graph(edit->new_graph_json, state.registry_);
                        if (g) {
                            std::lock_guard<std::mutex> lock(state.graph_mutex_);
                            state.pending_graph_ = std::move(g);
                        }
                    }
                }
                state.prev_frame_time_s_ = time_sec;

                state.scene_.update(time_sec);
                state.scene_.set_controller_poses(
                    lh ? std::optional<XrPosef>{lh->pose} : std::nullopt,
                    rh ? std::optional<XrPosef>{rh->pose} : std::nullopt);

                bool ok = state.renderer.render_eyes(
                    state.xrInstance, state.xrSession.get(),
                    state.xrSession.worldSpace_(), t,
                    [&](const Eigen::Matrix4f& proj, const Eigen::Matrix4f& view) {
                        const Eigen::Matrix4f pv = proj * view;

                        // Draw all graph nodes' render outputs
                        if (state.active_graph_) {
                            for (auto& call : state.active_graph_->draw_calls)
                                call(pv);
                        }

                        state.vr_editor_.draw(pv, state.text_mesh_);
                    });
                if (!ok) { return {}; }
                FrameLayers result;
                result.layers.at(0) = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&state.renderer.proj_layer());
                result.count = 1;
                return result;
            });
        }
    }

    LOG("android_main: exiting");
}
