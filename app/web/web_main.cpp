// Copyright 2026 Travis West
// Browser peer: the graph executor compiled to WASM, rendering into a
// WebGL2 canvas. Consumer-only (no HTTP server in a browser): connects to
// native peers over WebSocket and registers their advertised nodes as
// proxies, exactly like any other peer. Graph edits arrive through
// exported functions (web_set_graph) — callable from JS/devtools.
#include "component_registry.hpp"
#include "signal_graph.hpp"
#include "remote_node.hpp"
#include "eyeballs_node_abi.hpp"
#include "fly_camera.hpp"
#include "fly_camera_node.hpp"
#include "hand_node.hpp"
#include "math_nodes.hpp"
#include "cube_node.hpp"
#include "sky_dome.hpp"
#include "star_field.hpp"
#include "water_surface.hpp"
#include "sun_light.hpp"
#include "osc_node.hpp"
#include "dac_node.hpp"
#include "spectrogram.hpp"
#include "render_nodes.hpp"
#include "audio_region.hpp"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

namespace {

struct WebPeer {
    ComponentRegistry      registry;
    std::unique_ptr<Graph> active, pending;
    std::vector<std::unique_ptr<RemotePeer>>       peers;
    std::vector<RemotePeer*>                       awaiting_types;
    std::vector<std::unique_ptr<RemoteDescriptor>> remote_types;
    AudioRegion audio;
    // input
    bool  dragging = false;
    float yaw = 0.f, pitch = 0.f;
    bool  key[6] = {};  // W A S D Q E
    double last_t = 0.0;
    // graph JSON that references types still in flight (remote
    // advertisements); retried until it parses.
    std::string retry_graph;
    int         retry_cooldown = 0;
};
WebPeer g;

std::string query_param(const char* name) {
    char script[160];
    std::snprintf(script, sizeof(script),
        "decodeURIComponent((new URLSearchParams(location.search)).get('%s')||'')",
        name);
    const char* r = emscripten_run_script_string(script);
    return r ? r : "";
}

constexpr const char* kDefaultGraph = R"({
    "nodes":[
        {"id":"camera","type":"fly_camera","params":{"y":2,"z":8}},
        {"id":"sky","type":"sky_dome","params":{}},
        {"id":"water","type":"water_surface","params":{}},
        {"id":"sun","type":"sun_light","params":{}},
        {"id":"cube","type":"cube","params":{}}
    ],
    "edges":[{"from":"sky.sun_elevation_out","to":"water.sun_intensity"}]
})";

NodeInstance* find_camera() {
    if (!g.active) return nullptr;
    for (auto& n : g.active->nodes)
        if (std::string_view{n.desc->type_name} == "fly_camera") return &n;
    return nullptr;
}

void drive_camera(float dt) {
    NodeInstance* cam = find_camera();
    if (!cam || !cam->desc->set_scalar_in) return;
    auto* node = static_cast<FlyCameraNode*>(cam->data);
    node->inputs.yaw.value   = g.yaw;
    node->inputs.pitch.value = g.pitch;
    float fwd =  (g.key[0] ? 1.f : 0.f) - (g.key[2] ? 1.f : 0.f);
    float str =  (g.key[3] ? 1.f : 0.f) - (g.key[1] ? 1.f : 0.f);
    float up  =  (g.key[5] ? 1.f : 0.f) - (g.key[4] ? 1.f : 0.f);
    float sp = 6.f * dt;
    node->inputs.x.value += sp * (str * std::cos(g.yaw) - fwd * std::sin(g.yaw));
    node->inputs.z.value += sp * (-str * std::sin(g.yaw) - fwd * std::cos(g.yaw));
    node->inputs.y.value += sp * up;
}

// Peer handshakes + deferred graph application. Driven by a timer, not
// rAF — headless/offscreen browsers throttle rAF but keep timers alive.
void pump_network(void*) {
    for (auto it = g.awaiting_types.begin(); it != g.awaiting_types.end();) {
        RemotePeer* p = *it;
        if (!p->types_ready()) { ++it; continue; }
        int n = 0;
        for (const auto& ti : p->types()) {
            auto rd = std::make_unique<RemoteDescriptor>(
                p, ti.type + "@" + p->alias(), ti.type, ti.schema);
            g.registry.register_builtin(rd->descriptor());
            g.remote_types.push_back(std::move(rd));
            ++n;
        }
        std::printf("peer %s: %d remote types registered\n", p->alias().c_str(), n);
        it = g.awaiting_types.erase(it);
    }
    if (!g.retry_graph.empty()) {
        if (auto graph = parse_graph(g.retry_graph, g.registry)) {
            g.pending = std::move(graph);
            g.retry_graph.clear();
            std::printf("retry graph applied\n");
        }
    }
}

void frame() {
    double t = emscripten_get_now() / 1000.0;
    float dt = g.last_t > 0 ? float(t - g.last_t) : 0.016f;
    g.last_t = t;

    if (g.pending) {
        if (g.active) migrate_graph(*g.pending, *g.active);
        g.active = std::move(g.pending);
    }
    if (!g.active) return;
    if (!g.active->plan) g.audio.rebuild(*g.active);
    g.audio.publish(*g.active);

    int w = 0, h = 0;
    emscripten_get_canvas_element_size("#canvas", &w, &h);
    float aspect = h > 0 ? float(w) / float(h) : 1.f;
    if (NodeInstance* cam = find_camera())
        static_cast<FlyCameraNode*>(cam->data)->inputs.aspect.value = aspect;
    drive_camera(dt);

    glViewport(0, 0, w, h);
    glClearColor(0.05f, 0.05f, 0.1f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    tick_graph(*g.active, t);
    g.audio.capture_latches(*g.active);
    g.audio.pump_offline(dt);

    Eigen::Matrix4f pv;
    auto it = g.active->values.find("camera.pv");
    if (it != g.active->values.end() &&
        std::holds_alternative<Eigen::Matrix4f>(it->second)) {
        pv = std::get<Eigen::Matrix4f>(it->second);
    } else {
        FlyCamera fallback{};
        pv = fallback.proj(aspect) * fallback.view();
    }
    for (auto& call : g.active->draw_calls) call(pv);
}

EM_BOOL on_mouse(int type, const EmscriptenMouseEvent* e, void*) {
    if (type == EMSCRIPTEN_EVENT_MOUSEDOWN) g.dragging = true;
    else if (type == EMSCRIPTEN_EVENT_MOUSEUP) g.dragging = false;
    else if (type == EMSCRIPTEN_EVENT_MOUSEMOVE && g.dragging) {
        g.yaw   -= float(e->movementX) * 0.005f;
        g.pitch -= float(e->movementY) * 0.005f;
        if (g.pitch >  1.5f) g.pitch =  1.5f;
        if (g.pitch < -1.5f) g.pitch = -1.5f;
    }
    return EM_TRUE;
}

EM_BOOL on_key(int type, const EmscriptenKeyboardEvent* e, void*) {
    bool down = type == EMSCRIPTEN_EVENT_KEYDOWN;
    std::string_view k{e->key};
    int idx = -1;
    if (k == "w" || k == "W") idx = 0;
    else if (k == "a" || k == "A") idx = 1;
    else if (k == "s" || k == "S") idx = 2;
    else if (k == "d" || k == "D") idx = 3;
    else if (k == "q" || k == "Q") idx = 4;
    else if (k == "e" || k == "E") idx = 5;
    if (idx < 0) return EM_FALSE;
    g.key[idx] = down;
    return EM_TRUE;
}

} // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE void web_set_graph(const char* json) {
    if (auto graph = parse_graph(json, g.registry)) g.pending = std::move(graph);
    else std::printf("web_set_graph: parse failed\n");
}

EMSCRIPTEN_KEEPALIVE const char* web_get_graph() {
    static std::string out;
    out = g.active ? serialize_graph(*g.active) : "{}";
    return out.c_str();
}

EMSCRIPTEN_KEEPALIVE void web_connect_peer(const char* ws_url) {
    std::string alias = ws_url;
    if (auto p = alias.find("://"); p != std::string::npos) alias = alias.substr(p + 3);
    if (auto p = alias.find('/'); p != std::string::npos) alias = alias.substr(0, p);
    auto peer = std::make_unique<RemotePeer>(ws_url, alias);
    peer->request_types();
    g.awaiting_types.push_back(peer.get());
    g.peers.push_back(std::move(peer));
}

} // extern "C"

int main() {
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.majorVersion = 2;
    attrs.depth = EM_TRUE;
    auto ctx = emscripten_webgl_create_context("#canvas", &attrs);
    if (ctx <= 0) { std::printf("webgl2 context failed\n"); return 1; }
    emscripten_webgl_make_context_current(ctx);

    auto& reg = g.registry;
    reg.register_builtin(make_descriptor<FlyCameraNode>());
    reg.register_builtin(make_descriptor<HandNode>());
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
    reg.register_builtin(make_descriptor<CubeNode>());
    reg.register_builtin(make_descriptor<SkyDome>());
    reg.register_builtin(make_descriptor<StarField>());
    reg.register_builtin(make_descriptor<WaterSurface>());
    reg.register_builtin(make_descriptor<SunLight>());
    reg.register_builtin(make_descriptor<OscNode>());
    reg.register_builtin(make_descriptor<DacNode>());
    reg.register_builtin(make_descriptor<SpectrogramNode>());
    reg.register_builtin(make_descriptor<TextureViewNode>());

    g.active = parse_graph(kDefaultGraph, g.registry);

    emscripten_set_mousedown_callback("#canvas", nullptr, EM_TRUE, on_mouse);
    emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_TRUE, on_mouse);
    emscripten_set_mousemove_callback("#canvas", nullptr, EM_TRUE, on_mouse);
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, on_key);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, on_key);

    // ?peer=ws://host:port/ws connects on boot; ?graph=<urlencoded json>
    // applies once its types exist (remote advertisements arrive async).
    if (auto peer = query_param("peer"); !peer.empty()) web_connect_peer(peer.c_str());
    if (auto gj = query_param("graph"); !gj.empty()) g.retry_graph = gj;

    emscripten_set_interval(pump_network, 250, nullptr);
    emscripten_set_main_loop(frame, 0, EM_FALSE);
    return 0;
}
