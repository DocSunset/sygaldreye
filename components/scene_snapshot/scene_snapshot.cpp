#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "stb_image_write.h"
#pragma GCC diagnostic pop
#include "scene_snapshot.hpp"
#include <GLES3/gl3.h>
#include <cstdio>
#include <vector>

bool write_snapshot(SnapshotParams const& p,
                    std::function<void(Eigen::Matrix4f const&,
                                       Eigen::Matrix4f const&)> const& draw_fn,
                    std::string const& png_path)
{
    GLuint fbo = 0, color_rb = 0, depth_rb = 0;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &color_rb);
    glGenRenderbuffers(1, &depth_rb);

    glBindRenderbuffer(GL_RENDERBUFFER, color_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, p.width, p.height);

    glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, p.width, p.height);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_rb);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::fprintf(stderr, "scene_snapshot: framebuffer incomplete\n");
        glDeleteFramebuffers(1, &fbo);
        glDeleteRenderbuffers(1, &color_rb);
        glDeleteRenderbuffers(1, &depth_rb);
        return false;
    }

    glViewport(0, 0, p.width, p.height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw_fn(p.projection, p.view);

    std::vector<uint8_t> pixels(static_cast<size_t>(p.width) * p.height * 4);
    glReadPixels(0, 0, p.width, p.height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // Flip rows: OpenGL bottom-up → PNG top-down
    const int stride = p.width * 4;
    std::vector<uint8_t> flipped(pixels.size());
    for (int row = 0; row < p.height; ++row)
        std::copy(pixels.begin() + (p.height - 1 - row) * stride,
                  pixels.begin() + (p.height - 1 - row) * stride + stride,
                  flipped.begin() + row * stride);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &color_rb);
    glDeleteRenderbuffers(1, &depth_rb);

    int ok = stbi_write_png(png_path.c_str(), p.width, p.height, 4, flipped.data(), stride);
    if (!ok) {
        std::fprintf(stderr, "scene_snapshot: stbi_write_png failed for %s\n", png_path.c_str());
        return false;
    }
    return true;
}
