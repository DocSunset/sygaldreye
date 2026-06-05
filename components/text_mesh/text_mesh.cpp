#include "text_mesh.hpp"
#include "dejavu_sans_atlas.hpp"
#include "gl_program.hpp"
#include <GLES3/gl3.h>
#include <android/log.h>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#include "stb_image.h"
#pragma clang diagnostic pop

#define TAG  "text_mesh"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace {
constexpr const char* VERT = R"(#version 300 es
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
uniform mat4 uMVP;
out vec2 vUV;
void main() {
    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
    vUV = aUV;
}
)";

constexpr const char* FRAG = R"(#version 300 es
precision mediump float;
in vec2 vUV;
uniform sampler2D uAtlas;
uniform float uRange;
uniform vec4 uColor;
out vec4 fragColor;
float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}
void main() {
    vec3 msd = texture(uAtlas, vUV).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float dx = dFdx(sd), dy = dFdy(sd);
    float screen_px = uRange * length(vec2(dx, dy)) * 0.7071;
    float alpha = smoothstep(0.5 - screen_px, 0.5 + screen_px, sd);
    fragColor = vec4(uColor.rgb, uColor.a * alpha);
}
)";
} // namespace

TextMesh::TextMesh()  = default;
TextMesh::~TextMesh() {
    if (tex_ != 0U) { glDeleteTextures(1, &tex_); }
    if (vao_ != 0U) { glDeleteVertexArrays(1, &vao_); }
    if (vbo_ != 0U) { glDeleteBuffers(1, &vbo_); }
}
TextMesh::TextMesh(TextMesh&&) noexcept            = default;
TextMesh& TextMesh::operator=(TextMesh&&) noexcept = default;

void TextMesh::init() {
    auto res = GlProgram::build(VERT, FRAG);
    if (!res) { LOGE("shader build failed"); return; }
    prog_     = std::make_unique<GlProgram>(std::move(*res));
    mvp_loc_  = prog_->uniform_location("uMVP");
    tex_loc_  = prog_->uniform_location("uAtlas");
    range_loc_= prog_->uniform_location("uRange");
    color_loc_= prog_->uniform_location("uColor");

    const auto& info = dejavu_sans::ATLAS_INFO;
    int ww = 0, hh = 0, ch = 0;
    stbi_uc* px = stbi_load_from_memory(
        dejavu_sans::PNG_DATA, dejavu_sans::PNG_SIZE,
        &ww, &hh, &ch, 3);
    if (!px) { LOGE("atlas PNG decode failed"); return; }

    glGenTextures(1, &tex_);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, ww, hh, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(px);
    (void)info;

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray(0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);
}

void TextMesh::draw(std::string_view text, const Eigen::Matrix4f& mvp) const {
    if (!prog_ || tex_ == 0U || text.empty()) return;

    const auto& info = dejavu_sans::ATLAS_INFO;
    const float inv_w = 1.0F / static_cast<float>(info.width);
    const float inv_h = 1.0F / static_cast<float>(info.height);

    // Build quad data: 6 verts * 4 floats per glyph
    std::vector<float> verts;
    verts.reserve(text.size() * 24U);
    float cursor = 0.0F;
    for (unsigned char uch : text) {
        int cp = static_cast<int>(uch);
        if (cp < dejavu_sans::FIRST_CODEPOINT || cp > dejavu_sans::LAST_CODEPOINT) {
            cp = static_cast<int>('?');
        }
        const auto& g = dejavu_sans::GLYPHS[cp - dejavu_sans::FIRST_CODEPOINT];
        if (g.plane_right > g.plane_left) {
            float xl = cursor + g.plane_left;
            float xr = cursor + g.plane_right;
            float yb = g.plane_bottom;
            float yt = g.plane_top;
            float ul = g.atlas_left  * inv_w;
            float ur = g.atlas_right * inv_w;
            // atlas yOrigin is bottom; flip for GL texture coords (origin top-left)
            float vb = 1.0F - (g.atlas_bottom * inv_h);
            float vt = 1.0F - (g.atlas_top    * inv_h);
            // Two triangles: BL, BR, TR, BL, TR, TL
            float quad[24] = {
                xl, yb, ul, vb,
                xr, yb, ur, vb,
                xr, yt, ur, vt,
                xl, yb, ul, vb,
                xr, yt, ur, vt,
                xl, yt, ul, vt,
            };
            verts.insert(verts.end(), quad, quad + 24);
        }
        cursor += g.advance;
    }
    if (verts.empty()) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    prog_->use();
    GlProgram::uniform(mvp_loc_, mvp);
    glUniform1i(tex_loc_, 0);
    glUniform1f(range_loc_, info.distance_range);
    constexpr float white[4] = { 1.0F, 1.0F, 1.0F, 1.0F };
    glUniform4fv(color_loc_, 1, white);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
        verts.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size() / 4));
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
}
