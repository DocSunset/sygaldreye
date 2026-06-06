#include <android/log.h>
#include <android_native_app_glue.h>
#include <openxr/openxr.h>
#include "renderer.hpp"
#include "xr_session.hpp"
#include "frame_loop.hpp"
#include "scene.hpp"
#include "input.hpp"
#include "cube_mesh.hpp"
#include "text_mesh.hpp"
#include "water_surface.hpp"
#include "sky_dome.hpp"
#include "light.hpp"
#include "http_server.hpp"
#include "mdns_advertiser.hpp"
#include "param_registry.hpp"
#include "mic_capture.hpp"
#include "push_to_talk.hpp"
#include "eyeballs_node_abi.hpp"
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
#include <cmath>
#include <cstdio>
#include <ctime>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

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
    CubeMesh cube_mesh_{};
    TextMesh text_mesh_{};
    std::optional<WaterSurface>  water_{};
    std::optional<SkyDome>       sky_{};
    std::optional<MicCapture>    mic_{};
    PushToTalk                   push_to_talk_{};
    ComponentRegistry            registry_{};
    RdGpu                        rd_gpu_{};
    VrEditor                     vr_editor_{};
    bool                         prev_trigger_left_ = false;
    std::mutex                   graph_mutex_{};
    std::unique_ptr<Graph>       pending_graph_{};
    std::unique_ptr<Graph>       active_graph_{};
    std::string                  meta_graph_json_{};
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
    state.cube_mesh_.init();
    state.text_mesh_.init();
    state.input_.create(state.xrInstance, state.xrSession.get());

    WaterParams wp;
    wp.grid_w    = 64;
    wp.grid_h    = 64;
    wp.cell_size = 0.375f;
    state.water_ = WaterSurface::create(wp);

    SkyParams sp;
    sp.radius      = 500.0f;
    sp.star_count  = 2000;
    state.sky_ = SkyDome::create(sp);

    state.rd_gpu_.init(256, 256);
    state.registry_.register_builtin(make_descriptor<WaterSurface>());
    state.registry_.register_builtin(make_descriptor<SkyDome>());
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

    constexpr const char* kDefaultGraph = R"({
        "nodes":[
            {"id":"sky","type":"sky_dome","params":{}},
            {"id":"water","type":"water_surface","params":{}},
            {"id":"head","type":"head_pose","params":{}},
            {"id":"renderer","type":"renderer","params":{}}
        ],
        "edges":[
            {"from":"sky.sun_elevation_out","to":"water.sun intensity"}
        ]
    })";
    if (auto g = parse_graph(kDefaultGraph, state.registry_))
        state.active_graph_ = std::move(g);

    state.vr_editor_.init(state.registry_, state.active_graph_.get());

    constexpr const char* kCompanionUrl = "http://192.168.1.1:9090";
    state.push_to_talk_.set_companion_url(kCompanionUrl);
    LOG("push_to_talk: using fallback companion URL %s", kCompanionUrl);

    state.mic_ = MicCapture::create([&state](const float* samples, int frames) {
        state.push_to_talk_.feed(samples, frames);
    });
    if (state.mic_) {
        state.mic_->start();
        LOG("mic capture started");
    } else {
        LOGE("mic capture failed to create");
    }

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
        [&state](std::string_view body) -> std::string {
            auto g = parse_graph(std::string(body), state.registry_);
            if (!g) return R"({"ok":false,"error":"parse_graph failed"})";
            std::lock_guard<std::mutex> lock(state.graph_mutex_);
            state.pending_graph_ = std::move(g);
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
        [&state](std::string_view) -> std::string {
            if (state.water_) return to_json(*state.water_);
            return "{}";
        },
        [&state, &http_server](std::string_view body) -> std::string {
            if (state.water_) {
                from_json(*state.water_, body);
                http_server.broadcast_event("params", to_json(*state.water_));
            }
            return "{}";
        });
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
                        state.active_graph_ = std::move(state.pending_graph_);
                        state.vr_editor_.on_graph_changed(state.active_graph_.get());
                    }
                }
                if (state.active_graph_) tick_graph(*state.active_graph_, time_sec);
                if (!state.input_.sync(state.xrSession.get(), state.xrSession.worldSpace_(), t,
                                       state.xrSession.should_render())) {
                    LOGW("input sync failed — skipping input");
                }

                bool trigger_left = state.input_.trigger_pressed(Hand::LEFT);
                if (trigger_left && !state.prev_trigger_left_) {
                    state.push_to_talk_.begin_recording();
                    LOG("push_to_talk: recording started");
                } else if (!trigger_left && state.prev_trigger_left_) {
                    state.push_to_talk_.end_recording([](std::string_view text) {
                        LOG("transcript: %s", std::string(text).c_str());
                    });
                }
                state.prev_trigger_left_ = trigger_left;

                auto lh = state.input_.hand_pose(Hand::LEFT);
                auto rh = state.input_.hand_pose(Hand::RIGHT);

                // VR editor: palette + node card interaction
                {
                    const XrPosef* lp = lh ? &lh->pose : nullptr;
                    const XrPosef* rp = rh ? &rh->pose : nullptr;
                    bool tl = state.input_.trigger_pressed(Hand::LEFT);
                    bool tr = state.input_.trigger_pressed(Hand::RIGHT);
                    auto edit = state.vr_editor_.update(
                        lp, rp, tl, tr,
                        state.active_graph_.get(), state.registry_);
                    if (edit) {
                        auto g = parse_graph(edit->new_graph_json, state.registry_);
                        if (g) {
                            std::lock_guard<std::mutex> lock(state.graph_mutex_);
                            state.pending_graph_ = std::move(g);
                        }
                    }
                }

                state.scene_.update(time_sec);
                state.scene_.set_controller_poses(
                    lh ? std::optional<XrPosef>{lh->pose} : std::nullopt,
                    rh ? std::optional<XrPosef>{rh->pose} : std::nullopt);

                // Day-night cycle: 180-second period
                constexpr double kDayPeriod = 180.0;
                // +0.25 offsets the cycle so time 0 lands at solar noon (elev_norm = 1)
                float phase      = static_cast<float>(fmod(time_sec + kDayPeriod * 0.25, kDayPeriod) / kDayPeriod);
                float elev_norm  = sinf(2.0f * static_cast<float>(M_PI) * phase); // -1..1
                float elev_rad   = elev_norm * (static_cast<float>(M_PI) / 2.0f);
                float azimuth    = static_cast<float>(M_PI) * phase;
                float ce         = cosf(elev_rad);
                Eigen::Vector3f sun_dir{-ce * cosf(azimuth), -sinf(elev_rad), -ce * sinf(azimuth)};
                float warm       = std::max(0.0f, 1.0f - fabsf(elev_norm) * 3.5f);
                warm = warm * warm;
                Eigen::Vector3f sun_color{1.0f, 0.85f + 0.15f * (1.0f - warm), 0.65f + 0.35f * (1.0f - warm)};
                float sun_intensity = std::max(0.05f, sinf(elev_rad) * 1.3f);

                if (state.sky_) {
                    SkyParams sp;
                    sp.radius              = 500.0f;
                    sp.star_count          = 2000;
                    sp.sun_elevation       = elev_norm;
                    sp.body_dir            = -sun_dir;
                    sp.body_color          = {sun_color.x(), sun_color.y(), sun_color.z(), 1.0f};
                    sp.body_angular_radius = 0.014f;
                    state.sky_->set_params(sp);
                }

                Light sun_light;
                sun_light.direction = sun_dir;
                sun_light.color     = sun_color;
                sun_light.intensity = sun_intensity;
                if (state.water_) {
                    state.water_->set_sun(sun_light);
                    state.water_->update(static_cast<float>(time_sec));
                }

                // Water model: centered at user, 1.5m below floor
                constexpr float kWaterY    = -1.5f;
                constexpr float kCellSize  = 0.375f;
                constexpr int   kGridW     = 64;
                constexpr int   kGridH     = 64;
                Eigen::Matrix4f water_model = Eigen::Matrix4f::Identity();
                water_model(0,3) = -(kGridW - 1) * kCellSize * 0.5f;
                water_model(1,3) = kWaterY;
                water_model(2,3) = -(kGridH - 1) * kCellSize * 0.5f;

                bool ok = state.renderer.render_eyes(
                    state.xrInstance, state.xrSession.get(),
                    state.xrSession.worldSpace_(), t,
                    [&](const Eigen::Matrix4f& proj, const Eigen::Matrix4f& view) {
                        const Eigen::Matrix4f pv = proj * view;
                        const Eigen::Vector3f view_pos =
                            -(view.block<3,3>(0,0).transpose() * view.block<3,1>(0,3));

                        // Sky dome first (no depth write, always behind everything)
                        if (state.sky_) {
                            glDepthMask(GL_FALSE);
                            state.sky_->draw(pv);
                            glDepthMask(GL_TRUE);
                        }

                        state.cube_mesh_.begin_batch({&sun_light, 1}, view_pos);
                        for (const auto& cube : state.scene_.cubes()) {
                            state.cube_mesh_.draw(pv * cube.model, cube.model, cube.material);
                        }
                        state.cube_mesh_.end_batch();
                        for (const auto& lbl : state.scene_.labels()) {
                            state.text_mesh_.draw(lbl.text, pv * lbl.transform);
                        }

                        if (state.water_) {
                            state.water_->draw(pv * water_model, water_model, view_pos);
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
