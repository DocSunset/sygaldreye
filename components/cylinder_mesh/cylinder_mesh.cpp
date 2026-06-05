#include "cylinder_mesh.hpp"
#include <Eigen/Geometry>
#include <android/log.h>
#include <cmath>
#include <numbers>
#include <vector>

#define TAG "cylinder_mesh"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#define GL_CHECK(call) do { \
    (call); \
    GLenum _gl_err = glGetError(); \
    if (_gl_err != GL_NO_ERROR) { \
        LOGE("GL error 0x%x in " #call " (" __FILE__ ":%d)", (unsigned)_gl_err, __LINE__); \
    } \
} while(0)

namespace {
constexpr GLsizei VERTEX_STRIDE = 6 * static_cast<GLsizei>(sizeof(float));
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr)
const auto NORMAL_OFFSET = reinterpret_cast<const void*>(3 * sizeof(float));

struct Vert { float px, py, pz, nx, ny, nz; };
} // namespace

CylinderMesh CylinderMesh::create(int slices) {
    std::vector<Vert> verts;
    std::vector<uint16_t> idxs;
    const float step = (2.0F * std::numbers::pi_v<float>) / static_cast<float>(slices);

    // Side geometry: 2*slices vertices, slices*6 indices
    for (int i = 0; i < slices; ++i) {
        const float theta = static_cast<float>(i) * step;
        const float cos_t = std::cos(theta);
        const float sin_t = std::sin(theta);
        verts.push_back({cos_t,  sin_t,  0.5F, cos_t, sin_t, 0.0F}); // top
        verts.push_back({cos_t,  sin_t, -0.5F, cos_t, sin_t, 0.0F}); // bottom
    }
    for (int i = 0; i < slices; ++i) {
        const auto top0  = static_cast<uint16_t>((2 * i));
        const auto bot0  = static_cast<uint16_t>((2 * i) + 1);
        const auto top1  = static_cast<uint16_t>((2 * ((i + 1) % slices)));
        const auto bot1  = static_cast<uint16_t>((2 * ((i + 1) % slices)) + 1);
        idxs.insert(idxs.end(), {top0, bot0, top1, top1, bot0, bot1});
    }

    // Cap geometry: (1 + slices) verts per cap, slices triangles per cap
    for (int cap = 0; cap < 2; ++cap) {
        const float nz  = (cap == 0) ? 1.0F : -1.0F;
        const float z   = (cap == 0) ? 0.5F : -0.5F;
        const auto  ctr = static_cast<uint16_t>(verts.size());
        verts.push_back({0.0F, 0.0F, z, 0.0F, 0.0F, nz});
        const auto rim_start = static_cast<uint16_t>(verts.size());
        for (int i = 0; i < slices; ++i) {
            const float theta = static_cast<float>(i) * step;
            verts.push_back({std::cos(theta), std::sin(theta), z, 0.0F, 0.0F, nz});
        }
        for (int i = 0; i < slices; ++i) {
            const auto r0 = static_cast<uint16_t>(rim_start + i);
            const auto r1 = static_cast<uint16_t>(rim_start + ((i + 1) % slices));
            if (cap == 0) { idxs.insert(idxs.end(), {ctr, r0, r1}); }
            else          { idxs.insert(idxs.end(), {ctr, r1, r0}); }
        }
    }

    CylinderMesh mesh;
    mesh.index_count_ = static_cast<GLsizei>(idxs.size());
    GL_CHECK(glGenVertexArrays(1, &mesh.vao_));
    GL_CHECK(glGenBuffers(1, &mesh.vbo_));
    GL_CHECK(glGenBuffers(1, &mesh.ebo_));
    GL_CHECK(glBindVertexArray(mesh.vao_));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(verts.size() * sizeof(Vert)),
        verts.data(), GL_STATIC_DRAW));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo_));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(idxs.size() * sizeof(uint16_t)),
        idxs.data(), GL_STATIC_DRAW));
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_STRIDE, nullptr));
    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_STRIDE, NORMAL_OFFSET));
    glBindVertexArray(0);
    return mesh;
}

void CylinderMesh::draw() const {
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, index_count_, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}

CylinderMesh::~CylinderMesh() {
    if (vao_ != 0U) { glDeleteVertexArrays(1, &vao_); }
    if (vbo_ != 0U) { glDeleteBuffers(1, &vbo_); }
    if (ebo_ != 0U) { glDeleteBuffers(1, &ebo_); }
}

CylinderMesh::CylinderMesh(CylinderMesh&& src) noexcept
    : vao_(src.vao_), vbo_(src.vbo_), ebo_(src.ebo_), index_count_(src.index_count_) {
    src.vao_ = 0U; src.vbo_ = 0U; src.ebo_ = 0U; src.index_count_ = 0;
}

CylinderMesh& CylinderMesh::operator=(CylinderMesh&& src) noexcept {
    if (this != &src) {
        if (vao_ != 0U) { glDeleteVertexArrays(1, &vao_); }
        if (vbo_ != 0U) { glDeleteBuffers(1, &vbo_); }
        if (ebo_ != 0U) { glDeleteBuffers(1, &ebo_); }
        vao_ = src.vao_; src.vao_ = 0U;
        vbo_ = src.vbo_; src.vbo_ = 0U;
        ebo_ = src.ebo_; src.ebo_ = 0U;
        index_count_ = src.index_count_; src.index_count_ = 0;
    }
    return *this;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Eigen::Matrix4f cylinder_transform(Eigen::Vector3f point_a,
                                    Eigen::Vector3f point_b,
                                    float radius) {
    Eigen::Vector3f axis = point_b - point_a;
    float len = axis.norm();
    Eigen::Vector3f dir = axis / len;
    Eigen::Quaternionf rot = Eigen::Quaternionf::FromTwoVectors(Eigen::Vector3f::UnitZ(), dir);
    Eigen::Affine3f xf = Eigen::Translation3f(((point_a + point_b) * 0.5F)) * rot
                         * Eigen::Scaling(radius, radius, len);
    return xf.matrix();
}
