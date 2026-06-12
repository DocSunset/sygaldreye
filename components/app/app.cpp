#include <android/log.h>
#include <android_native_app_glue.h>
#include <openxr/openxr.h>
#include <time.h>
#include "renderer.hpp"
#include "xr_session.hpp"
#include "frame_loop.hpp"
#include "scene.hpp"
#include "input.hpp"
#include "peer_core.hpp"
#include "cube_node.hpp"
#include "text_mesh.hpp"
#include "mdns_advertiser.hpp"
#include "eyeballs_node_abi.hpp"
#include "speech_to_text.hpp"
#include "stt_whisper.hpp"
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
#include "wav_player.hpp"
#include "osc_node.hpp"
#include "dac_node.hpp"
#include "ugens.hpp"
#include "mesh_nodes.hpp"
#include "spatialize_node.hpp"
#include "sample_player.hpp"
#include "spectrogram.hpp"
#include "poke_stick.hpp"
#include "poke_button.hpp"
#include <GLES3/gl3.h>
#include <memory>
#include <optional>
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
    XrInstance   xrInstance = XR_NULL_HANDLE;
    XrSystemId   xrSystemId = XR_NULL_SYSTEM_ID;
    Renderer     renderer{};
    XrSessionObj xrSession{};
    FrameLoop    frame_loop_{};
    Scene        scene_{};
    Input        input_{};
    TextMesh     text_mesh_{};
    PeerCore     core_{};
    VrEditor     vr_editor_{};
    double       prev_frame_time_s_ = 0.0;
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

static void register_device_nodes(ComponentRegistry& reg) {
    reg.register_builtin(make_descriptor<WaterSurface>());
    reg.register_builtin(make_descriptor<SkyDome>());
    reg.register_builtin(make_descriptor<StarField>());
    reg.register_builtin(make_descriptor<RdGpu>());
    reg.register_builtin(make_descriptor<HeadPoseNode>());
    reg.register_builtin(make_descriptor<LeftControllerNode>());
    reg.register_builtin(make_descriptor<RightControllerNode>());
    reg.register_builtin(make_descriptor<AtmosSynth>());
    reg.register_builtin(make_descriptor<RainSynth>());
    reg.register_builtin(make_descriptor<FireSynth>());
    reg.register_builtin(make_descriptor<EngineSynth>());
    reg.register_builtin(make_descriptor<WaterSynth>());
    reg.register_builtin(make_descriptor<CreatureSynth>());
    reg.register_builtin(make_descriptor<ChimeSynth>());
    reg.register_builtin(make_descriptor<Lissajous>());
    reg.register_builtin(make_descriptor<Aurora>());
    reg.register_builtin(make_descriptor<Chladni>());
    reg.register_builtin(make_descriptor<TerrainRenderer>());
    reg.register_builtin(make_descriptor<ParticleSystem>());
    reg.register_builtin(make_descriptor<ReactionDiffusion>());
    reg.register_builtin(make_descriptor<RendererNode>());
    reg.register_builtin(make_descriptor<SunLight>());
    reg.register_builtin(make_descriptor<TriggerEdge>());
    reg.register_builtin(make_descriptor<PttGate>());
    reg.register_builtin(make_descriptor<MicInputNode>());
    reg.register_builtin(make_descriptor<CubeNode>());
    reg.register_builtin(make_descriptor<TextLabelNode>());
    reg.register_builtin(make_descriptor<SpeechToTextNode>());
    reg.register_builtin(make_descriptor<SttWhisperNode>());
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
    reg.register_builtin(make_descriptor<UdpSendNode>());
    reg.register_builtin(make_descriptor<UdpRecvNode>());
    reg.register_builtin(make_descriptor<UiSliderNode>());
    reg.register_builtin(make_descriptor<UiButtonNode>());
    reg.register_builtin(make_descriptor<UiPaneNode>());
    reg.register_builtin(make_descriptor<HandNode>());
    reg.register_builtin(make_descriptor<FlyCameraNode>());
    reg.register_builtin(make_descriptor<SpawnerNode>());
    reg.register_builtin(make_descriptor<AuroraCurtainNode>());
    reg.register_builtin(make_descriptor<WavPlayerNode>());
    reg.register_builtin(make_descriptor<OscNode>());
    reg.register_builtin(make_descriptor<DacNode>());
    reg.register_builtin(make_descriptor<NoiseNode>());
    reg.register_builtin(make_descriptor<AdsrNode>());
    reg.register_builtin(make_descriptor<VcaNode>());
    reg.register_builtin(make_descriptor<MixNode>());
    reg.register_builtin(make_descriptor<ScatterNode>());
    reg.register_builtin(make_descriptor<MeshGridNode>());
    reg.register_builtin(make_descriptor<MeshSphereNode>());
    reg.register_builtin(make_descriptor<MeshBoxNode>());
    reg.register_builtin(make_descriptor<MeshCylinderNode>());
    reg.register_builtin(make_descriptor<MeshRenderNode>());
    reg.register_builtin(make_descriptor<MeshInstancesNode>());
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
    reg.register_builtin(make_descriptor<SpectrogramNode>());
    reg.register_builtin(make_descriptor<PokeStickNode>());
    reg.register_builtin(make_descriptor<PokeButtonNode>());
}

// Pump XR source nodes with current frame's live pose/input data.
static void pump_xr_sources(AppState& state, double /*time_sec*/) {
    Graph* g = state.core_.graph();
    if (!g) return;
    auto lh = state.input_.hand_pose(Hand::LEFT);
    auto rh = state.input_.hand_pose(Hand::RIGHT);
    bool trigger_left  = state.input_.trigger_pressed(Hand::LEFT);
    bool trigger_right = state.input_.trigger_pressed(Hand::RIGHT);
    float grip_l = state.input_.grip_pressed(Hand::LEFT)  ? 1.f : 0.f;
    float grip_r = state.input_.grip_pressed(Hand::RIGHT) ? 1.f : 0.f;
    Eigen::Vector2f th_l = state.input_.thumbstick(Hand::LEFT);
    Eigen::Vector2f th_r = state.input_.thumbstick(Hand::RIGHT);

    for (auto& n : g->nodes) {
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
                p, trigger_left, grip_l, th_l.x(), th_l.y(),
                state.input_.button1_pressed(Hand::LEFT),
                state.input_.button2_pressed(Hand::LEFT));
        } else if (type == "right_controller") {
            XrPosef p = rh ? rh->pose : XrPosef{{0,0,0,1},{0,0,0}};
            static_cast<RightControllerNode*>(n.data)->set_state(
                p, trigger_right, grip_r, th_r.x(), th_r.y(),
                state.input_.button1_pressed(Hand::RIGHT),
                state.input_.button2_pressed(Hand::RIGHT));
        }
    }
}

void android_main(struct android_app* app) {
    LOG("android_main: started");
    acquire_multicast_lock(app);

    AppState state;
    app->userData  = &state;
    app->onAppCmd  = onAppCmd;

    // EGL and the core come up FIRST and never depend on XR: a launch with
    // the headset un-worn or controllers asleep used to leave a zombie
    // process (loader init / xrGetSystem fail when the form factor is
    // unavailable, everything cascaded). Now the peer is fully alive —
    // HTTP, graph, offline audio — and XR attaches whenever the headset
    // becomes available.
    auto binding = state.renderer.init();
    if (!binding) { LOGE("renderer init failed"); return; }
    state.text_mesh_.init();

    register_device_nodes(state.core_.registry);

    PeerCore::Config cfg;
    cfg.http_port = 8080;
    cfg.default_graph_json = R"({
        "nodes":[
            {"id":"sky","type":"sky_dome","params":{}},
            {"id":"water","type":"water_surface","params":{}},
            {"id":"head","type":"head_pose","params":{}},
            {"id":"renderer","type":"renderer","params":{}},
            {"id":"sun","type":"sun_light","params":{}},
            {"id":"cube","type":"cube","params":{}}
        ],
        "edges":[
            {"from":"sky.sun_elevation_out","to":"water.sun_intensity"}
        ]
    })";
    cfg.data_dir   = app->activity->internalDataPath;
    cfg.graphs_dir = std::string(app->activity->internalDataPath) + "/graphs";
    state.core_.on_graph_swapped = [&state](const Graph* g) {
        state.vr_editor_.on_graph_changed(g);
    };
    state.core_.init(cfg);

    state.vr_editor_.init(state.core_.registry, state.core_.graph());

    MdnsAdvertiser mdns_advertiser;
    mdns_advertiser.start("eyeballs", 8080);

    // GET /screenshot capture: runs at the renderer's post-resolve point,
    // where the eye image is guaranteed readable.
    state.renderer.post_resolve_hook = [&state](int w, int h) {
        state.core_.fulfil_screenshot(w, h);
    };

    // Drive the graph without an XR session (waiting for the headset, or
    // doffed mid-run): same core service as a frame, minus input/render.
    auto offline_tick = [&state]() {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        double now = double(ts.tv_sec) + double(ts.tv_nsec) * 1e-9;
        state.core_.begin_frame();
        state.core_.pump_contexts(1.f);
        state.core_.tick(now);
        state.core_.collect_edits();
    };

    // ── XR attach: poll until the runtime and headset are available ─────
    int xr_wait_ticks = 0;
    while (!app->destroyRequested) {
        if (state.xrInstance == XR_NULL_HANDLE)
            state.xrInstance = xr_create_instance(app);
        if (state.xrInstance != XR_NULL_HANDLE) {
            state.xrSystemId = xr_get_system(state.xrInstance);
            if (state.xrSystemId != XR_NULL_SYSTEM_ID) break;
        }
        if (xr_wait_ticks++ % 30 == 0)
            LOG("waiting for XR (headset worn? runtime up?) — peer is live meanwhile");
        int events;
        android_poll_source* source;
        if (ALooper_pollOnce(33, nullptr, &events,
                             reinterpret_cast<void**>(&source)) >= 0 && source)
            source->process(app, source);
        offline_tick();
    }
    if (app->destroyRequested) { LOG("android_main: exiting (pre-XR)"); return; }

    state.xrSession.create(state.xrInstance, state.xrSystemId, &binding->xr_binding);
    state.renderer.create_swapchains(state.xrInstance, state.xrSystemId, state.xrSession.get());
    state.input_.create(state.xrInstance, state.xrSession.get());

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
        if (!state.xrSession.session_running()) {
            // Doffed / session idle: the peer stays fully alive.
            offline_tick();
        }
        if (state.xrSession.session_running()) {
            state.frame_loop_.run_frame(state.xrSession.get(), [&](XrTime t) -> FrameLayers {
                double time_sec = static_cast<double>(t) * 1e-9;

                state.core_.begin_frame();   // swap+migrate, apply params
                state.core_.pump_contexts(1.f);

                if (!state.input_.sync(state.xrSession.get(), state.xrSession.worldSpace_(), t,
                                       state.xrSession.should_render())) {
                    LOGW("input sync failed — skipping input");
                }

                auto lh = state.input_.hand_pose(Hand::LEFT);
                auto rh = state.input_.hand_pose(Hand::RIGHT);

                pump_xr_sources(state, time_sec);
                state.core_.tick(time_sec);

                // VR editor: palette, node cards, drag-to-connect, sliders,
                // delete/undo. Edits flow through the core's queue mapping.
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
                        state.core_.graph(), state.core_.registry);
                    if (edit) state.core_.queue_edit(std::move(edit->new_graph_json));
                }
                state.core_.collect_edits();
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
                        if (Graph* g = state.core_.graph())
                            for (auto& call : g->draw_calls) call(pv);
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
