// Copyright 2026 Travis West
#include "host_app.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <GLES3/gl3.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>

namespace {

// Returns the number following "key": in json, or fallback if absent.
double find_number(std::string_view json, std::string_view key, double fallback) {
    std::string needle = "\"" + std::string(key) + "\"";
    auto p = json.find(needle);
    if (p == std::string_view::npos) return fallback;
    p = json.find(':', p);
    if (p == std::string_view::npos) return fallback;
    return std::strtod(std::string(json.substr(p + 1, 32)).c_str(), nullptr);
}

std::string camera_json(const FlyCamera& c) {
    char buf[160];
    std::snprintf(buf, sizeof(buf),
        R"({"x":%.3f,"y":%.3f,"z":%.3f,"yaw":%.4f,"pitch":%.4f,"fov":%.4f})",
        double(c.pos.x()), double(c.pos.y()), double(c.pos.z()),
        double(c.yaw), double(c.pitch), double(c.fov));
    return buf;
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
    http_.add_route("GET", "/camera", [this](std::string_view) -> std::string {
        return camera_json(camera());
    });
    http_.add_route("POST", "/camera", [this](std::string_view body) -> std::string {
        FlyCamera c = camera();
        c.pos.x() = float(find_number(body, "x",     c.pos.x()));
        c.pos.y() = float(find_number(body, "y",     c.pos.y()));
        c.pos.z() = float(find_number(body, "z",     c.pos.z()));
        c.yaw     = float(find_number(body, "yaw",   c.yaw));
        c.pitch   = float(find_number(body, "pitch", c.pitch));
        c.fov     = float(find_number(body, "fov",   c.fov));
        set_camera(c);
        return camera_json(c);
    });
    // Blocks until the render thread captures the next frame (2 s timeout).
    http_.add_route("POST", "/screenshot", [this](std::string_view body) -> std::string {
        std::string path = "/tmp/sygaldreye_shot.png";
        auto p = body.find("\"path\"");
        if (p != std::string_view::npos) {
            auto a = body.find('"', body.find(':', p));
            auto b = body.find('"', a + 1);
            if (a != std::string_view::npos && b != std::string_view::npos)
                path = std::string(body.substr(a + 1, b - a - 1));
        }
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
