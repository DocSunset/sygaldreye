// Copyright 2026 Travis West
#include "mesh_nodes.hpp"
#include <GLES3/gl3.h>
#include <cstdio>

// ── mesh_grid ───────────────────────────────────────────────────────────────

void MeshGridNode::operator()(double) {
    int   n = int(inputs.cells.value);
    float s = inputs.size.value;
    if (n != cells_ || s != size_) {
        auto m = std::make_shared<TriMeshData>();
        m->vertices.reserve(size_t(n + 1) * (n + 1));
        for (int z = 0; z <= n; ++z)
            for (int x = 0; x <= n; ++x)
                m->vertices.push_back({{(float(x) / n - 0.5f) * s, 0.f,
                                        (float(z) / n - 0.5f) * s},
                                       {0.f, 1.f, 0.f},
                                       {1.f, 1.f, 1.f, 1.f}});
        m->indices.reserve(size_t(n) * n * 6);
        for (int z = 0; z < n; ++z)
            for (int x = 0; x < n; ++x) {
                uint32_t a = uint32_t(z * (n + 1) + x), w = uint32_t(n + 1);
                m->indices.insert(m->indices.end(),
                                  {a, a + w, a + 1, a + 1, a + w, a + w + 1});
            }
        cached_ = std::move(m);
        cells_ = n; size_ = s;
    }
    outputs.mesh.value = cached_;
}

// ── mesh_displace ───────────────────────────────────────────────────────────

void MeshDisplaceNode::operator()(double) {
    const MeshPtr& in = inputs.mesh.value;
    const GpuTexture& tex = inputs.texture.value;
    if (!in || !tex.valid()) { outputs.mesh.value = in; return; }

    // GPU → CPU: read the texture back through a cached FBO.
    if (fbo_ == 0) glGenFramebuffers(1, &fbo_);
    GLint prev_fbo = 0; glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, tex.id, 0);
    bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (ok) {
        tw_ = tex.width; th_ = tex.height;
        pixels_.resize(size_t(tw_) * th_ * 4);
        glReadPixels(0, 0, tw_, th_, GL_RGBA, GL_UNSIGNED_BYTE, pixels_.data());
    }
    glBindFramebuffer(GL_FRAMEBUFFER, GLuint(prev_fbo));
    if (!ok || tw_ == 0) { outputs.mesh.value = in; return; }

    // CPU: displace along +Y by luminance; tint vertex colors from texture.
    Eigen::Vector2f lo{1e9f, 1e9f}, hi{-1e9f, -1e9f};
    for (const auto& v : in->vertices) {
        Eigen::Vector2f xz{v.position.x(), v.position.z()};
        lo = lo.cwiseMin(xz);
        hi = hi.cwiseMax(xz);
    }
    Eigen::Vector2f span = (hi - lo).cwiseMax(Eigen::Vector2f::Constant(1e-6f));

    auto out = std::make_shared<TriMeshData>(*in);
    for (auto& v : out->vertices) {
        float u = (v.position.x() - lo.x()) / span.x();
        float w = (v.position.z() - lo.y()) / span.y();
        int px = std::min(tw_ - 1, int(u * float(tw_)));
        int py = std::min(th_ - 1, int(w * float(th_)));
        const unsigned char* p = &pixels_[(size_t(py) * tw_ + px) * 4];
        float r = p[0] / 255.f, g = p[1] / 255.f, b = p[2] / 255.f;
        float lum = 0.299f * r + 0.587f * g + 0.114f * b;
        v.position.y() += lum * inputs.amplitude.value;
        float t = inputs.tint.value;
        v.color.head<3>() = (1.f - t) * v.color.head<3>() +
                            t * Eigen::Vector3f{r, g, b};
    }
    // Recompute smooth normals.
    for (auto& v : out->vertices) v.normal.setZero();
    for (size_t i = 0; i + 2 < out->indices.size(); i += 3) {
        auto& a = out->vertices[out->indices[i]];
        auto& b2 = out->vertices[out->indices[i + 1]];
        auto& c = out->vertices[out->indices[i + 2]];
        Eigen::Vector3f fn = (b2.position - a.position)
                                 .cross(c.position - a.position);
        a.normal += fn; b2.normal += fn; c.normal += fn;
    }
    for (auto& v : out->vertices)
        if (v.normal.squaredNorm() > 1e-12f) v.normal.normalize();

    outputs.mesh.value = std::move(out);
}

MeshDisplaceNode::~MeshDisplaceNode() {
    if (fbo_) glDeleteFramebuffers(1, &fbo_);
}

// ── mesh_render ─────────────────────────────────────────────────────────────

namespace {
constexpr const char* kVert = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec4 aColor;
uniform mat4 uMVP;
uniform vec3 uOffset;
out vec3 vNormal;
out vec4 vColor;
void main() {
    vNormal = aNormal;
    vColor  = aColor;
    gl_Position = uMVP * vec4(aPos + uOffset, 1.0);
}
)";
constexpr const char* kFrag = R"(#version 300 es
precision mediump float;
in vec3 vNormal;
in vec4 vColor;
uniform vec3 uLightDir;
out vec4 fragColor;
void main() {
    float diff = max(dot(normalize(vNormal), normalize(-uLightDir)), 0.0);
    fragColor = vec4(vColor.rgb * (0.25 + 0.75 * diff), vColor.a);
}
)";
} // namespace

void MeshRenderNode::operator()(double) {
    held_ = inputs.mesh.value;
    outputs.render.value = [this](const Eigen::Matrix4f& pv) {
        if (!held_) return;
        if (!prog_) {
            auto p = GlProgram::build(kVert, kFrag);
            if (!p) { std::fprintf(stderr, "mesh_render: shader failed\n"); return; }
            prog_ = std::make_unique<GlProgram>(std::move(*p));
        }
        if (uploaded_ != held_.get()) {
            if (uploaded_ == nullptr) gpu_ = TriMesh::create(*held_);
            else gpu_.update(*held_);
            uploaded_ = held_.get();
        }
        prog_->use();
        GlProgram::uniform(prog_->uniform_location("uMVP"), pv);
        glUniform3f(prog_->uniform_location("uOffset"),
                    inputs.x.value, inputs.y.value, inputs.z.value);
        const auto& ld = inputs.light_dir.value;
        glUniform3f(prog_->uniform_location("uLightDir"), ld.x(), ld.y(), ld.z());
        gpu_.draw();
    };
}
