// Copyright 2025 Travis West
#pragma once
#include <functional>
#include <optional>

// Called each frame with (width, height); should issue GL draw calls.
using RenderCallback = std::function<void(int width, int height)>;

struct HostGlWindow {
    static std::optional<HostGlWindow> create(int width, int height, const char* title);

    // Run the render loop until window is closed.
    void run(RenderCallback callback);

    // Underlying GLFWwindow*, for input polling by callers that link GLFW.
    void* native() const;

    ~HostGlWindow();
    HostGlWindow(HostGlWindow&&) noexcept;
    HostGlWindow& operator=(HostGlWindow&&) noexcept;
    HostGlWindow(const HostGlWindow&) = delete;
    HostGlWindow& operator=(const HostGlWindow&) = delete;

private:
    struct Impl;
    Impl* impl_ = nullptr;
    explicit HostGlWindow(Impl*);
};
