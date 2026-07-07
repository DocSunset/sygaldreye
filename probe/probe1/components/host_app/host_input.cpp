// Copyright 2026 Travis West
#include "host_input.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdio>

#include "fly_camera.hpp"
#include "host_app.hpp"

void apply_fps_input(void* window, HostApp& app, float dt) {
    auto* win = static_cast<GLFWwindow*>(window);
    constexpr float kSpeed = 6.f;    // m/s
    constexpr float kSens = 0.003f;  // rad/pixel

    FlyCamera cam{};
    if (auto p = app.read_node_output("camera", "pos", "vec3"))
        cam.pos = std::get<Eigen::Vector3f>(*p);
    if (auto y = app.read_node_output("camera", "yaw_out", "scalar"))
        cam.yaw = float(std::get<double>(*y));
    if (auto p = app.read_node_output("camera", "pitch_out", "scalar"))
        cam.pitch = float(std::get<double>(*p));

    float fwd = 0.f, side = 0.f, up = 0.f;
    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) fwd += 1.f;
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) fwd -= 1.f;
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) side += 1.f;
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) side -= 1.f;
    if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) up += 1.f;
    if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) up -= 1.f;
    cam.pos +=
        (cam.forward() * fwd + cam.right() * side + Eigen::Vector3f::UnitY() * up) * kSpeed * dt;

    static double last_x = 0.0, last_y = 0.0;
    static bool looking = false;
    bool moved = fwd != 0.f || side != 0.f || up != 0.f;
    if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        double x, y;
        glfwGetCursorPos(win, &x, &y);
        if (looking) {
            cam.yaw -= float(x - last_x) * kSens;
            cam.pitch -= float(y - last_y) * kSens;
            cam.pitch = std::clamp(cam.pitch, -1.5f, 1.5f);
            moved = true;
        }
        last_x = x;
        last_y = y;
        looking = true;
        glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else if (looking) {
        looking = false;
        glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    if (!moved) return;
    char params[160];
    std::snprintf(
        params,
        sizeof(params),
        R"({"x":%g,"y":%g,"z":%g,"yaw":%g,"pitch":%g})",
        double(cam.pos.x()),
        double(cam.pos.y()),
        double(cam.pos.z()),
        double(cam.yaw),
        double(cam.pitch));
    app.queue_param("camera", params);
}
