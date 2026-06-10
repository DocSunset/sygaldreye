// Copyright 2026 Travis West
// Host app: live signal graph + HTTP control surface.
// Windowed (GLFW, FPS controls) by default; --headless renders to an
// offscreen FBO via EGL so agents can run instances anywhere.
#include "host_app.hpp"
#include "host_input.hpp"
#include "host_gl_window.hpp"
#include "host_gl_context.hpp"
#include <GLES3/gl3.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

namespace {

double now_s() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

int run_headless(HostApp& app, int port, int w, int h) {
    auto ctx = HostGlContext::create();
    if (!ctx) { std::puts("headless: EGL context failed"); return 1; }
    app.init(port);  // after context: some nodes compile shaders in constructors

    GLuint fbo, color, depth;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &color);
    glGenRenderbuffers(1, &depth);
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::puts("headless: framebuffer incomplete");
        return 1;
    }

    std::printf("headless: running at %dx%d\n", w, h);
    const double t0 = now_s();
    while (!app.quit_requested()) {
        app.frame(w, h, now_s() - t0);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    bool headless = false;
    int  port     = 8080;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--headless") == 0) headless = true;
        else if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) port = std::atoi(argv[++i]);
    }

    HostApp app;
    if (headless) return run_headless(app, port, 1280, 720);

    auto win = HostGlWindow::create(1280, 720, "sygaldreye");
    if (!win) { std::puts("could not create window"); return 1; }
    app.init(port);  // after context: some nodes compile shaders in constructors

    const double t0 = now_s();
    double prev = 0.0;
    win->run([&](int w, int h) {
        double t = now_s() - t0;
        FlyCamera cam = app.camera();
        apply_fps_input(win->native(), cam, float(t - prev));
        app.set_camera(cam);
        prev = t;
        app.frame(w, h, t);
        if (app.quit_requested()) std::exit(0);
    });
    return 0;
}
