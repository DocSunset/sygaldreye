// Copyright 2025 Travis West
#include "host_gl_window.hpp"
#include <GLFW/glfw3.h>
#include <utility>

struct HostGlWindow::Impl {
    GLFWwindow* window = nullptr;
    explicit Impl(GLFWwindow* w) : window(w) {}
    ~Impl() {
        if (window) glfwDestroyWindow(window);
        glfwTerminate();
    }
};

HostGlWindow::HostGlWindow(Impl* impl) : impl_(impl) {}

HostGlWindow::HostGlWindow(HostGlWindow&& other) noexcept
    : impl_(std::exchange(other.impl_, nullptr)) {}

HostGlWindow& HostGlWindow::operator=(HostGlWindow&& other) noexcept {
    if (this != &other) {
        delete impl_;
        impl_ = std::exchange(other.impl_, nullptr);
    }
    return *this;
}

HostGlWindow::~HostGlWindow() { delete impl_; }

std::optional<HostGlWindow> HostGlWindow::create(int width, int height, const char* title) {
    if (!glfwInit()) return std::nullopt;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!win) { glfwTerminate(); return std::nullopt; }

    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    return HostGlWindow{new Impl{win}};
}

void HostGlWindow::run(RenderCallback callback) {
    if (!impl_ || !impl_->window) return;
    GLFWwindow* win = impl_->window;
    while (!glfwWindowShouldClose(win)) {
        int w = 0;
        int h = 0;
        glfwGetFramebufferSize(win, &w, &h);
        callback(w, h);
        glfwSwapBuffers(win);
        glfwPollEvents();
    }
}
