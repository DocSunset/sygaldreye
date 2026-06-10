// Copyright 2026 Travis West
#include "host_app.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <GLES3/gl3.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

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
        } else {
            std::snprintf(buf, sizeof(buf), R"("audio")");
        }
        return buf;
    }, val);
}

} // namespace

void HostApp::install_routes() {
    http_.add_route("GET", "/graph", [this](std::string_view) -> std::string {
        std::lock_guard<std::mutex> lock(graph_mutex_);
        if (active_) return serialize_graph(*active_);
        return R"({"nodes":[],"edges":[]})";
    });
    http_.add_route("POST", "/graph", [this](std::string_view body) -> std::string {
        auto g = parse_graph(std::string(body), registry_);
        if (!g) return R"({"ok":false,"error":"parse_graph failed"})";
        std::lock_guard<std::mutex> lock(graph_mutex_);
        pending_ = std::move(g);
        return R"({"ok":true})";
    });
    http_.add_route("GET", "/palette", [this](std::string_view) -> std::string {
        std::string out = "[";
        bool first = true;
        for (const auto& type_name : registry_.type_names()) {
            const auto* desc = registry_.find(type_name);
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
    // Generic param injection: {"node":"<id>","params":{...}} — works on any
    // node's inputs. /camera and /controller are sugar over this.
    http_.add_route("POST", "/param", [this](std::string_view body) -> std::string {
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
    // Blocks until the render thread captures the next frame (2 s timeout).
    http_.add_route("POST", "/screenshot", [this](std::string_view body) -> std::string {
        std::string path = "/tmp/sygaldreye_shot.png";
        if (auto p = find_string(body, "path"); !p.empty()) path = std::string(p);
        std::unique_lock<std::mutex> lock(shot_mutex_);
        shot_path_ = path;
        shot_done_ = false;
        bool ok = shot_cv_.wait_for(lock, std::chrono::seconds(2),
                                    [this] { return shot_done_; }) && shot_ok_;
        shot_path_.clear();
        if (!ok) return R"({"ok":false,"error":"capture timed out or failed"})";
        return std::string(R"({"ok":true,"path":")") + path + "\"}";
    });
    http_.add_route("POST", "/quit", [this](std::string_view) -> std::string {
        quit_.store(true);
        return R"({"ok":true})";
    });
}

void HostApp::fulfil_screenshot(int width, int height) {
    std::lock_guard<std::mutex> lock(shot_mutex_);
    if (shot_path_.empty() || shot_done_) return;
    std::vector<unsigned char> pixels(size_t(width) * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    std::vector<unsigned char> flipped(pixels.size());
    for (int row = 0; row < height; ++row)
        std::copy_n(&pixels[size_t(height - 1 - row) * width * 4],
                    size_t(width) * 4, &flipped[size_t(row) * width * 4]);
    shot_ok_   = stbi_write_png(shot_path_.c_str(), width, height, 4,
                                flipped.data(), width * 4) != 0;
    shot_done_ = true;
    shot_cv_.notify_all();
}
