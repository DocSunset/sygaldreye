#pragma once
#include <vector>
#include <cstdint>
#include <Eigen/Core>
#include <GLES3/gl3.h>

struct TriVertex {
    Eigen::Vector3f position;
    Eigen::Vector3f normal;
    Eigen::Vector4f color;
};

struct TriMeshData {
    std::vector<TriVertex> vertices;
    std::vector<uint32_t>  indices;
};

struct TriMesh {
    static TriMesh create(TriMeshData const&);

    TriMesh() = default;
    ~TriMesh();
    TriMesh(TriMesh const&) = delete;
    TriMesh& operator=(TriMesh const&) = delete;
    TriMesh(TriMesh&&) noexcept;
    TriMesh& operator=(TriMesh&&) noexcept;

    void update(TriMeshData const&);
    void draw() const;

private:
    GLuint vao_          = 0;
    GLuint vbo_          = 0;
    GLuint ebo_          = 0;
    GLsizei index_count_ = 0;
    GLsizei vertex_count_ = 0;
};
