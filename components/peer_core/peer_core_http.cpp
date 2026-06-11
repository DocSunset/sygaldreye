// Copyright 2026 Travis West
// The HTTP control surface — identical on every peer.
#include "peer_core.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>

namespace {

double find_number(std::string_view json, std::string_view key, double fallback) {
    std::string needle = "\"" + std::string(key) + "\"";
    auto p = json.find(needle);
    if (p == std::string_view::npos) return fallback;
    p = json.find(':', p);
    if (p == std::string_view::npos) return fallback;
    return std::strtod(std::string(json.substr(p + 1, 32)).c_str(), nullptr);
}

std::string_view find_string(std::string_view json, std::string_view key) {
    std::string needle = "\"" + std::string(key) + "\":\"";
    auto p = json.find(needle);
    if (p == std::string_view::npos) return {};
    p += needle.size();
    auto e = json.find('"', p);
    return (e == std::string_view::npos) ? std::string_view{} : json.substr(p, e - p);
}

std::string_view find_object(std::string_view json, std::string_view key) {
    std::string needle = "\"" + std::string(key) + "\":{";
    auto p = json.find(needle);
    if (p == std::string_view::npos) return {};
    p += needle.size() - 1;
    int depth = 0;
    for (auto i = p; i < json.size(); ++i) {
        if (json[i] == '{') ++depth;
        else if (json[i] == '}' && --depth == 0) return json.substr(p, i - p + 1);
    }
    return {};
}

std::string value_json(const PortValue& val) {
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        char buf[160];
        if constexpr (std::is_same_v<T, double>) {
            std::snprintf(buf, sizeof(buf), "%g", v);
        } else if constexpr (std::is_same_v<T, Eigen::Vector2f>) {
            std::snprintf(buf, sizeof(buf), "[%g,%g]", double(v.x()), double(v.y()));
        } else if constexpr (std::is_same_v<T, Eigen::Vector3f>) {
            std::snprintf(buf, sizeof(buf), "[%g,%g,%g]", double(v.x()), double(v.y()), double(v.z()));
        } else if constexpr (std::is_same_v<T, Eigen::Vector4f> ||
                             std::is_same_v<T, Eigen::Quaternionf>) {
            std::snprintf(buf, sizeof(buf), "[%g,%g,%g,%g]", double(v.x()), double(v.y()), double(v.z()), double(v.w()));
        } else if constexpr (std::is_same_v<T, Eigen::Matrix4f>) {
            std::snprintf(buf, sizeof(buf), R"("matrix4")");
        } else if constexpr (std::is_same_v<T, GpuTexture>) {
            std::snprintf(buf, sizeof(buf), R"("texture:%u")", v.id);
        } else if constexpr (std::is_same_v<T, DrawFn>) {
            std::snprintf(buf, sizeof(buf), R"("drawfn")");
        } else if constexpr (std::is_same_v<T, MeshPtr>) {
            std::snprintf(buf, sizeof(buf), R"("mesh:%zuv")", v ? v->vertices.size() : 0);
        } else if constexpr (std::is_same_v<T, AudioBuffer>) {
            std::snprintf(buf, sizeof(buf), R"("audio[%d]")", v.frames);
        } else {
            std::snprintf(buf, sizeof(buf), R"("value")");
        }
        return buf;
    }, val);
}

} // namespace

void PeerCore::install_routes() {
    http_.add_route("GET", "/graph", [this](std::string_view) -> std::string {
        std::lock_guard<std::mutex> lock(graph_mutex_);
        if (active_) return serialize_graph(*active_);
        return R"({"nodes":[],"edges":[]})";
    });
    http_.add_route("POST", "/graph", [this](std::string_view body) -> std::string {
        auto g = parse_graph(std::string(body), registry);
        if (!g) return R"({"ok":false,"error":"parse_graph failed"})";
        auto graph_json = serialize_graph(*g);
        {
            std::lock_guard<std::mutex> lock(graph_mutex_);
            pending_ = std::move(g);
        }
        http_.broadcast_event("graph", graph_json);
        return R"({"ok":true})";
    });
    http_.add_route("GET", "/palette", [this](std::string_view) -> std::string {
        std::string out = "[";
        bool first = true;
        for (const auto& type_name : registry.type_names()) {
            const auto* desc = registry.find(type_name);
            if (!desc) continue;
            if (!first) out += ',';
            first = false;
            out += "{\"type\":\""; out += type_name; out += '"';
            if (desc->create && desc->serialize) {
                void* tmp = desc->create();
                const char* s = desc->serialize(tmp);
                out += ",\"defaults\":"; out += s;
                if (desc->free_str) desc->free_str(s);
                desc->destroy(tmp);
            }
            out += '}';
        }
        return out + "]";
    });
    http_.add_route("GET", "/plugins", [this](std::string_view) -> std::string {
        std::string out = "[";
        bool first = true;
        for (const auto& n : registry.type_names()) {
            if (!first) out += ',';
            first = false;
            out += '"'; out += n; out += '"';
        }
        return out + "]";
    });
    http_.add_route("POST", "/plugins", [this](std::string_view body) -> std::string {
        char path[512];
        std::snprintf(path, sizeof(path), "%s/plugin_%ld.so", cfg_.data_dir.c_str(),
                      static_cast<long>(std::time(nullptr)));
        FILE* f = std::fopen(path, "wb");
        if (!f) return R"({"ok":false,"error":"fopen failed"})";
        std::fwrite(body.data(), 1, body.size(), f);
        std::fclose(f);
        if (!registry.load_plugin(path))
            return R"({"ok":false,"error":"load_plugin failed"})";
        // Hot reload: re-instantiate the running graph through the (possibly
        // replaced) registry. Untouched types adopt their live state via
        // migration; a reloaded type gets fresh instances of the new code
        // with its current params carried through the serialization.
        {
            std::lock_guard<std::mutex> lock(graph_mutex_);
            if (active_) queue_edit(serialize_graph(*active_));
        }
        return R"({"ok":true})";
    });
    // Generic param injection: {"node":"<id>","params":{...}} — works on any
    // node's inputs. /camera, /controller and /play are sugar over this.
    http_.add_route("POST", "/param", [this](std::string_view raw) -> std::string {
        std::string body = compact_json(std::string(raw));
        auto node = find_string(body, "node");
        auto params = find_object(body, "params");
        if (node.empty() || params.empty())
            return R"({"ok":false,"error":"need node and params"})";
        queue_param(std::string(node), std::string(params));
        return R"({"ok":true})";
    });
    http_.add_route("GET", "/values", [this](std::string_view) -> std::string {
        std::lock_guard<std::mutex> lock(values_mutex_);
        std::string out = "{";
        bool first = true;
        for (const auto& [key, val] : values_snapshot_) {
            if (!first) out += ',';
            first = false;
            out += '"'; out += key; out += "\":";
            out += value_json(val);
        }
        return out + "}";
    });
    http_.add_route("GET", "/camera", [this](std::string_view) -> std::string {
        std::string out = "{";
        for (const char* k : {"pos", "yaw", "pitch"}) {
            if (auto v = probe(std::string("camera.") + k)) {
                if (out.size() > 1) out += ',';
                out += '"'; out += k; out += "\":"; out += value_json(*v);
            }
        }
        return out + "}";
    });
    http_.add_route("POST", "/camera", [this](std::string_view body) -> std::string {
        queue_param("camera", std::string(body));  // keys match camera inputs
        return R"({"ok":true})";
    });
    http_.add_route("POST", "/controller", [this](std::string_view body) -> std::string {
        bool right = find_string(body, "hand") != "left";
        // Re-emit only numeric fields: hand node inputs share these names.
        char params[256];
        std::snprintf(params, sizeof(params),
            R"({"x":%g,"y":%g,"z":%g,"qx":%g,"qy":%g,"qz":%g,"qw":%g,"trigger":%g,"grip":%g,"thumb_x":%g,"thumb_y":%g})",
            find_number(body, "x", 0), find_number(body, "y", 1.2),
            find_number(body, "z", -0.4),
            find_number(body, "qx", 0), find_number(body, "qy", 0),
            find_number(body, "qz", 0), find_number(body, "qw", 1),
            find_number(body, "trigger", 0), find_number(body, "grip", 0),
            find_number(body, "thumb_x", 0), find_number(body, "thumb_y", 0));
        queue_param(right ? "hand_r" : "hand_l", params);
        return R"({"ok":true})";
    });
    http_.add_route("POST", "/play", [this](std::string_view body) -> std::string {
        char path[512];
        std::snprintf(path, sizeof(path), "%s/play.wav", cfg_.data_dir.c_str());
        FILE* f = std::fopen(path, "wb");
        if (!f) return R"({"ok":false,"error":"fopen"})";
        std::fwrite(body.data(), 1, body.size(), f);
        std::fclose(f);
        char params[576];
        std::snprintf(params, sizeof(params),
                      R"({"file":"%s","seq":%ld})", path, ++play_seq_);
        queue_param("speaker", params);
        return R"({"ok":true})";
    });
    // Speech ingress: WAV body → file + param event on the "stt" node
    // (file+seq), symmetric with /play → "speaker". A whisper_stt node
    // with id "stt" picks it up on the worker region.
    http_.add_route("POST", "/transcribe", [this](std::string_view body) -> std::string {
        char path[512];
        std::snprintf(path, sizeof(path), "%s/transcribe.wav", cfg_.data_dir.c_str());
        FILE* f = std::fopen(path, "wb");
        if (!f) return R"({"ok":false,"error":"fopen"})";
        std::fwrite(body.data(), 1, body.size(), f);
        std::fclose(f);
        char params[576];
        std::snprintf(params, sizeof(params),
                      R"({"file":"%s","seq":%ld})", path, ++play_seq_);
        queue_param("stt", params);
        return R"({"ok":true})";
    });
    // Blocks until the render thread captures the next frame (2 s timeout).
    auto request_shot = [this](const std::string& path) -> bool {
        std::unique_lock<std::mutex> lock(shot_mutex_);
        shot_path_ = path;
        shot_done_ = false;
        bool ok = shot_cv_.wait_for(lock, std::chrono::seconds(2),
                                    [this] { return shot_done_; }) && shot_ok_;
        shot_path_.clear();
        return ok;
    };
    http_.add_route("POST", "/screenshot", [this, request_shot](std::string_view body) -> std::string {
        std::string path = cfg_.data_dir + "/shot.png";
        if (auto p = find_string(body, "path"); !p.empty()) path = std::string(p);
        if (!request_shot(path))
            return R"({"ok":false,"error":"capture timed out or failed"})";
        return std::string(R"({"ok":true,"path":")") + path + "\"}";
    });
    http_.add_route("GET", "/screenshot", [this, request_shot](std::string_view) -> std::string {
        std::string path = cfg_.data_dir + "/eye.png";
        if (!request_shot(path)) return R"({"ok":false,"error":"capture timed out"})";
        std::string png;
        if (FILE* f = std::fopen(path.c_str(), "rb")) {
            char buf[4096];
            size_t n;
            while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) png.append(buf, n);
            std::fclose(f);
        }
        return png.empty() ? R"({"ok":false,"error":"read failed"})" : png;
    });
    http_.add_route("GET", "/meta-graph", [this](std::string_view) -> std::string {
        std::lock_guard<std::mutex> lock(meta_mutex_);
        return meta_graph_json_.empty() ? "{}" : meta_graph_json_;
    });
    http_.add_route("POST", "/meta-graph", [this](std::string_view body) -> std::string {
        std::lock_guard<std::mutex> lock(meta_mutex_);
        meta_graph_json_ = std::string(body);
        return R"({"ok":true})";
    });
    // Net mapping, consumer side: {"url":"ws://host:port/ws"} → register
    // that peer's advertised node types as local proxies.
    http_.add_route("POST", "/peer", [this](std::string_view body) -> std::string {
        auto url = find_string(body, "url");
        if (url.empty()) return R"({"ok":false,"error":"need url"})";
        int n = connect_peer(std::string(url));
        if (n == 0) return R"({"ok":false,"error":"no types fetched"})";
        char out[64];
        std::snprintf(out, sizeof(out), R"({"ok":true,"types":%d})", n);
        return out;
    });
    http_.add_route("POST", "/quit", [this](std::string_view) -> std::string {
        quit_.store(true);
        return R"({"ok":true})";
    });
}
