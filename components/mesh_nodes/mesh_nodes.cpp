// Copyright 2026 Travis West
#include "mesh_nodes.hpp"
#include <GLES3/gl3.h>
#include <cmath>
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

// ── generators ──────────────────────────────────────────────────────────────

void MeshSphereNode::operator()(double) {
    float r = inputs.radius.value;
    int   n = int(inputs.segments.value);
    if (r != radius_ || n != segs_) {
        auto m = std::make_shared<TriMeshData>();
        int lat = n / 2, lon = n;
        for (int i = 0; i <= lat; ++i) {
            float v = float(i) / lat, phi = v * float(M_PI);
            for (int j = 0; j <= lon; ++j) {
                float u = float(j) / lon, th = u * 2.f * float(M_PI);
                Eigen::Vector3f nrm{std::sin(phi) * std::cos(th), std::cos(phi),
                                    std::sin(phi) * std::sin(th)};
                m->vertices.push_back({nrm * r, nrm, {1, 1, 1, 1}});
            }
        }
        for (int i = 0; i < lat; ++i)
            for (int j = 0; j < lon; ++j) {
                uint32_t a = uint32_t(i * (lon + 1) + j), w = uint32_t(lon + 1);
                m->indices.insert(m->indices.end(),
                                  {a, a + w, a + 1, a + 1, a + w, a + w + 1});
            }
        cached_ = std::move(m);
        radius_ = r; segs_ = n;
    }
    outputs.mesh.value = cached_;
}

void MeshBoxNode::operator()(double) {
    Eigen::Vector3f s{inputs.sx.value, inputs.sy.value, inputs.sz.value};
    if (s != size_) {
        auto m = std::make_shared<TriMeshData>();
        const Eigen::Vector3f n[6] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
        for (int f = 0; f < 6; ++f) {
            Eigen::Vector3f u = (f < 2) ? Eigen::Vector3f{0,1,0} : Eigen::Vector3f{1,0,0};
            Eigen::Vector3f v = n[f].cross(u);
            uint32_t base = uint32_t(m->vertices.size());
            for (int k = 0; k < 4; ++k) {
                float du = (k == 1 || k == 2) ? 1.f : -1.f;
                float dv = (k >= 2) ? 1.f : -1.f;
                Eigen::Vector3f p = (n[f] + u * du + v * dv).cwiseProduct(s * 0.5f);
                m->vertices.push_back({p, n[f], {1, 1, 1, 1}});
            }
            m->indices.insert(m->indices.end(),
                              {base, base + 1, base + 2, base, base + 2, base + 3});
        }
        cached_ = std::move(m);
        size_ = s;
    }
    outputs.mesh.value = cached_;
}

void MeshCylinderNode::operator()(double) {
    float r = inputs.radius.value, h = inputs.height.value;
    int   n = int(inputs.segments.value);
    if (r != radius_ || h != height_ || n != segs_) {
        auto m = std::make_shared<TriMeshData>();
        for (int i = 0; i <= 1; ++i)
            for (int j = 0; j <= n; ++j) {
                float th = float(j) / n * 2.f * float(M_PI);
                Eigen::Vector3f nrm{std::cos(th), 0.f, std::sin(th)};
                m->vertices.push_back({{nrm.x() * r, (i - 0.5f) * h, nrm.z() * r},
                                       nrm, {1, 1, 1, 1}});
            }
        for (int j = 0; j < n; ++j) {
            uint32_t a = uint32_t(j), w = uint32_t(n + 1);
            m->indices.insert(m->indices.end(),
                              {a, a + 1, a + w, a + 1, a + w + 1, a + w});
        }
        cached_ = std::move(m);
        radius_ = r; height_ = h; segs_ = n;
    }
    outputs.mesh.value = cached_;
}

// ── deformers ───────────────────────────────────────────────────────────────

void MeshRippleNode::operator()(double t) {
    const MeshPtr& in = inputs.mesh.value;
    if (!in) { outputs.mesh.value = in; return; }
    auto out = std::make_shared<TriMeshData>(*in);
    float a = inputs.amplitude.value, f = inputs.freq.value;
    float ph = float(t) * inputs.speed.value;
    for (auto& v : out->vertices) {
        float w = std::sin(v.position.x() * f + ph) *
                  std::cos(v.position.z() * f * 0.7f + ph * 1.3f);
        v.position += v.normal * (a * w);
    }
    outputs.mesh.value = std::move(out);
}

void MeshTwistNode::operator()(double) {
    const MeshPtr& in = inputs.mesh.value;
    if (!in) { outputs.mesh.value = in; return; }
    auto out = std::make_shared<TriMeshData>(*in);
    float k = inputs.angle.value;
    for (auto& v : out->vertices) {
        float ang = v.position.y() * k;
        float c = std::cos(ang), s = std::sin(ang);
        float x = v.position.x(), z = v.position.z();
        v.position.x() = c * x - s * z;
        v.position.z() = s * x + c * z;
        float nx = v.normal.x(), nz = v.normal.z();
        v.normal.x() = c * nx - s * nz;
        v.normal.z() = s * nx + c * nz;
    }
    outputs.mesh.value = std::move(out);
}

void MeshTransformNode::operator()(double) {
    const MeshPtr& in = inputs.mesh.value;
    if (!in) { outputs.mesh.value = in; return; }
    auto out = std::make_shared<TriMeshData>(*in);
    const Eigen::Matrix4f& m4 = inputs.matrix.value;
    Eigen::Matrix3f rot = m4.block<3, 3>(0, 0);
    for (auto& v : out->vertices) {
        v.position = (m4 * v.position.homogeneous()).head<3>();
        v.normal   = (rot * v.normal).normalized();
    }
    outputs.mesh.value = std::move(out);
}
