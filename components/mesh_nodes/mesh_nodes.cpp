// Copyright 2026 Travis West
#include "mesh_nodes.hpp"
#include <GLES3/gl3.h>
#include <cmath>
#include <cstdio>

// ── mesh_grid ───────────────────────────────────────────────────────────────

void MeshGridNode::operator()(double) {
    int   n = int(endpoints.cells.get());
    float s = endpoints.size.get();
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
    endpoints.mesh.value = cached_;
}

// ── mesh_displace ───────────────────────────────────────────────────────────

void MeshDisplaceNode::operator()(double) {
    MeshPtr in = endpoints.mesh.get();
    GpuTexture tex = endpoints.texture.get();
    if (!in || !tex.valid()) { endpoints.mesh_out.value = in; return; }

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
    if (!ok || tw_ == 0) { endpoints.mesh_out.value = in; return; }

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
        v.position.y() += lum * endpoints.amplitude.get();
        float t = endpoints.tint.get();
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

    endpoints.mesh_out.value = std::move(out);
}

MeshDisplaceNode::~MeshDisplaceNode() {
    if (fbo_) glDeleteFramebuffers(1, &fbo_);
}

// ── generators ──────────────────────────────────────────────────────────────

void MeshSphereNode::operator()(double) {
    float r = endpoints.radius.get();
    int   n = int(endpoints.segments.get());
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
    endpoints.mesh.value = cached_;
}

void MeshBoxNode::operator()(double) {
    Eigen::Vector3f s{endpoints.sx.get(), endpoints.sy.get(), endpoints.sz.get()};
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
    endpoints.mesh.value = cached_;
}

void MeshCylinderNode::operator()(double) {
    float r = endpoints.radius.get(), h = endpoints.height.get();
    int   n = int(endpoints.segments.get());
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
    endpoints.mesh.value = cached_;
}

// ── deformers ───────────────────────────────────────────────────────────────

void MeshRippleNode::operator()(double t) {
    MeshPtr in = endpoints.mesh.get();
    if (!in) { endpoints.mesh_out.value = in; return; }
    auto out = std::make_shared<TriMeshData>(*in);
    float a = endpoints.amplitude.get(), f = endpoints.freq.get();
    float ph = float(t) * endpoints.speed.get();
    for (auto& v : out->vertices) {
        float w = std::sin(v.position.x() * f + ph) *
                  std::cos(v.position.z() * f * 0.7f + ph * 1.3f);
        v.position += v.normal * (a * w);
    }
    endpoints.mesh_out.value = std::move(out);
}

void MeshTwistNode::operator()(double) {
    MeshPtr in = endpoints.mesh.get();
    if (!in) { endpoints.mesh_out.value = in; return; }
    auto out = std::make_shared<TriMeshData>(*in);
    float k = endpoints.angle.get();
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
    endpoints.mesh_out.value = std::move(out);
}

void MeshTransformNode::operator()(double) {
    MeshPtr in = endpoints.mesh.get();
    if (!in) { endpoints.mesh_out.value = in; return; }
    auto out = std::make_shared<TriMeshData>(*in);
    Eigen::Matrix4f m4 = endpoints.matrix.get();
    Eigen::Matrix3f rot = m4.block<3, 3>(0, 0);
    for (auto& v : out->vertices) {
        v.position = (m4 * v.position.homogeneous()).head<3>();
        v.normal   = (rot * v.normal).normalized();
    }
    endpoints.mesh_out.value = std::move(out);
}

// ── span era ─────────────────────────────────────────────────────────────────

void ScatterNode::operator()(double) {
    int   n = std::max(1, int(endpoints.count.get()));
    float r = endpoints.radius.get();
    float s = endpoints.seed.get();
    float y = endpoints.y.get();
    if (n != n_ || r != r_ || s != s_ || y != y_) {
        pts_.resize(std::size_t(n) * 3);
        uint32_t rng = 0x9e3779b9u + uint32_t(s * 977.f);
        auto next = [&rng] {
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            return float(rng) / 4294967296.f;   // [0,1)
        };
        for (int i = 0; i < n; ++i) {
            float a = next() * 6.2831853f;
            float d = std::sqrt(next()) * r;    // uniform over the disc
            pts_[std::size_t(i) * 3 + 0] = std::cos(a) * d;
            pts_[std::size_t(i) * 3 + 1] = y;
            pts_[std::size_t(i) * 3 + 2] = std::sin(a) * d;
        }
        n_ = n; r_ = r; s_ = s; y_ = y;
    }
    endpoints.positions.value = Span{pts_.data(), n, 3};
}

void SeedsNode::operator()(double) {
    int   n = std::max(1, int(endpoints.count.get()));
    float b = endpoints.base.get(), s = endpoints.step.get();
    if (n != n_ || b != base_ || s != step_) {
        vals_.resize(std::size_t(n));
        for (int i = 0; i < n; ++i) vals_[std::size_t(i)] = b + float(i) * s;
        n_ = n; base_ = b; step_ = s;
    }
    endpoints.seeds.value = Span{vals_.data(), n, 1};
}
