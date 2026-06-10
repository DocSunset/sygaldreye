#pragma once
#include <Eigen/Core>
#include <GLES3/gl3.h>
#include <memory>
#include <string_view>

struct GlProgram;

/// Renders text strings as MSDF textured quads.
/// Emissive (unlit): text is always uniformly bright.
/// Call init() once after GL context is available, then draw() per frame.
struct TextMesh {
    TextMesh();
    ~TextMesh();
    TextMesh(const TextMesh&)            = delete;
    TextMesh& operator=(const TextMesh&) = delete;
    TextMesh(TextMesh&&) noexcept;
    TextMesh& operator=(TextMesh&&) noexcept;

    void init();

    /// Draw text with the given world-space MVP transform.
    /// Text baseline starts at the origin; glyphs extend in +X.
    /// Scale via the transform; one em unit = whatever the transform says.
    void draw(std::string_view text, const Eigen::Matrix4f& mvp) const;

private:
    std::unique_ptr<GlProgram> prog_;
    GLuint tex_   = 0U;
    GLuint vao_   = 0U;
    GLuint vbo_   = 0U;
    GLint  mvp_loc_   = -1;
    GLint  tex_loc_   = -1;
    GLint  range_loc_ = -1;
    GLint  color_loc_ = -1;
};
