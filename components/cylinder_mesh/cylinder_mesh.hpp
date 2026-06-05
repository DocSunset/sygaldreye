#pragma once
#include <Eigen/Core>
#include <GLES3/gl3.h>
#include <cstdint>

struct CylinderMesh {
    static CylinderMesh create(int slices);
    void draw() const;

    ~CylinderMesh();
    CylinderMesh(const CylinderMesh&) = delete;
    CylinderMesh& operator=(const CylinderMesh&) = delete;
    CylinderMesh(CylinderMesh&&) noexcept;
    CylinderMesh& operator=(CylinderMesh&&) noexcept;

private:
    CylinderMesh() = default;
    GLuint  vao_         = 0;
    GLuint  vbo_         = 0;
    GLuint  ebo_         = 0;
    GLsizei index_count_ = 0;
};

// Returns model matrix mapping unit cylinder onto the segment from a to b at given radius.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- a,b are positionally distinct by semantics
Eigen::Matrix4f cylinder_transform(Eigen::Vector3f point_a,
                                    Eigen::Vector3f point_b,
                                    float radius);
