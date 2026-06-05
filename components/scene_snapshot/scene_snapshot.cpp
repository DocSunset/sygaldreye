// Copyright 2025 Travis West
#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "stb_image_write.h"
#pragma GCC diagnostic pop
#include "scene_snapshot.hpp"
#include <GLES3/gl3.h>
#include <cstdio>
#include <vector>

namespace {

struct Fbo {
    GLuint fbo = 0, color_rb = 0, depth_rb = 0;
    bool ok = false;
};

static Fbo make_fbo(int w, int h) {
    Fbo f;
    glGenFramebuffers(1, &f.fbo);
    glGenRenderbuffers(1, &f.color_rb);
    glGenRenderbuffers(1, &f.depth_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, f.color_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, w, h);
    glBindRenderbuffer(GL_RENDERBUFFER, f.depth_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, f.fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, f.color_rb);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_RENDERBUFFER, f.depth_rb);
    f.ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (!f.ok) std::fprintf(stderr, "scene_snapshot: framebuffer incomplete\n");
    return f;
}

static void free_fbo(Fbo const& f) {
    glDeleteFramebuffers(1, &f.fbo);
    glDeleteRenderbuffers(1, &f.color_rb);
    glDeleteRenderbuffers(1, &f.depth_rb);
}

static void flip_rows(uint8_t* dst, uint8_t const* src, int w, int h) {
    int stride = w * 4;
    for (int row = 0; row < h; ++row)
        std::copy(src + (h - 1 - row) * stride, src + (h - row) * stride, dst + row * stride);
}

static void png_to_file(void* ctx, void* data, int size) {
    fwrite(data, 1, static_cast<size_t>(size), static_cast<FILE*>(ctx));
}

} // namespace

bool write_snapshot(SnapshotParams const& p, DrawFn const& draw_fn, std::string const& png_path)
{
    Fbo f = make_fbo(p.width, p.height);
    if (!f.ok) { free_fbo(f); return false; }

    glViewport(0, 0, p.width, p.height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw_fn(p.projection, p.view);

    std::vector<uint8_t> pixels(static_cast<size_t>(p.width) * p.height * 4);
    glReadPixels(0, 0, p.width, p.height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    std::vector<uint8_t> flipped(pixels.size());
    flip_rows(flipped.data(), pixels.data(), p.width, p.height);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    free_fbo(f);

    int ok = stbi_write_png(png_path.c_str(), p.width, p.height, 4, flipped.data(), p.width * 4);
    if (!ok) std::fprintf(stderr, "scene_snapshot: stbi_write_png failed for %s\n", png_path.c_str());
    return ok != 0;
}

bool write_animation(SnapshotParams const& base, float duration_s, int fps,
                     DrawFnTime const& draw_fn, std::string const& mp4_path)
{
    Fbo f = make_fbo(base.width, base.height);
    if (!f.ok) { free_fbo(f); return false; }
    glViewport(0, 0, base.width, base.height);

    char cmd[1024];
    std::snprintf(cmd, sizeof(cmd),
        "ffmpeg -y -f image2pipe -vcodec png -framerate %d -i pipe:0 "
        "-vcodec libx264 -pix_fmt yuv420p -movflags +faststart '%s' 2>/dev/null",
        fps, mp4_path.c_str());
    FILE* pipe = popen(cmd, "w");
    if (!pipe) {
        std::fprintf(stderr, "scene_snapshot: popen ffmpeg failed\n");
        free_fbo(f); return false;
    }

    std::vector<uint8_t> pixels(static_cast<size_t>(base.width) * base.height * 4);
    std::vector<uint8_t> flipped(pixels.size());
    int nframes = static_cast<int>(duration_s * fps);
    for (int i = 0; i < nframes; ++i) {
        float t = base.time_s + static_cast<float>(i) / static_cast<float>(fps);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        draw_fn(base.projection, base.view, t);
        glReadPixels(0, 0, base.width, base.height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        flip_rows(flipped.data(), pixels.data(), base.width, base.height);
        stbi_write_png_to_func(png_to_file, pipe, base.width, base.height, 4, flipped.data(), base.width * 4);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    free_fbo(f);
    int ret = pclose(pipe);
    if (ret != 0) std::fprintf(stderr, "scene_snapshot: ffmpeg exited with %d\n", ret);
    return ret == 0;
}
