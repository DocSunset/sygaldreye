// Copyright 2026 Travis West
#include "host_input.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

void apply_fps_input(void* window, FlyCamera& cam, float dt) {
    auto* win = static_cast<GLFWwindow*>(window);
    constexpr float kSpeed = 6.f;        // m/s
    constexpr float kSens  = 0.003f;     // rad/pixel

    float fwd = 0.f, side = 0.f, up = 0.f;
    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) fwd  += 1.f;
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) fwd  -= 1.f;
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) side += 1.f;
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) side -= 1.f;
    if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) up   += 1.f;
    if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) up   -= 1.f;
    cam.pos += (cam.forward() * fwd + cam.right() * side +
                Eigen::Vector3f::UnitY() * up) * kSpeed * dt;

    static double last_x = 0.0, last_y = 0.0;
    static bool   looking = false;
    if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        double x, y;
        glfwGetCursorPos(win, &x, &y);
        if (looking) {
            cam.yaw   -= float(x - last_x) * kSens;
            cam.pitch -= float(y - last_y) * kSens;
            cam.pitch  = std::clamp(cam.pitch, -1.5f, 1.5f);
        }
        last_x = x; last_y = y;
        looking = true;
        glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else if (looking) {
        looking = false;
        glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}
