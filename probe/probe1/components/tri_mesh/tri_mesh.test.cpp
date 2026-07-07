#include "tri_mesh.hpp"
#include "egl_context.hpp"
#include <GLES3/gl3.h>
#include <gtest/gtest.h>

namespace {
TriMeshData one_triangle() {
    TriMeshData d;
    d.vertices = {
        { {0.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 0.f, 0.f, 1.f} },
        { {1.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 1.f, 0.f, 1.f} },
        { {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 0.f, 1.f, 1.f} },
    };
    d.indices = { 0, 1, 2 };
    return d;
}
TriMeshData two_triangles() {
    TriMeshData d;
    d.vertices = {
        { {0.f,  0.f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f, 1.f, 1.f} },
        { {1.f,  0.f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f, 1.f, 1.f} },
        { {0.f,  1.f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f, 1.f, 1.f} },
        { {1.f,  1.f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f, 1.f, 1.f} },
    };
    d.indices = { 0, 1, 2, 1, 3, 2 };
    return d;
}
}

class TriMeshTest : public ::testing::Test {
protected:
    EglContext ctx_;
    void SetUp() override { ASSERT_TRUE(ctx_.init()); }
};

TEST_F(TriMeshTest, CreateAndDrawNoGlError) {
    glGetError(); // clear
    auto mesh = TriMesh::create(one_triangle());
    EXPECT_EQ(glGetError(), GLenum(GL_NO_ERROR));
    mesh.draw();
    EXPECT_EQ(glGetError(), GLenum(GL_NO_ERROR));
}

TEST_F(TriMeshTest, UpdateSameSize) {
    auto mesh = TriMesh::create(one_triangle());
    glGetError();
    mesh.update(one_triangle());
    mesh.draw();
    EXPECT_EQ(glGetError(), GLenum(GL_NO_ERROR));
}

TEST_F(TriMeshTest, UpdateDifferentSize) {
    auto mesh = TriMesh::create(one_triangle());
    glGetError();
    mesh.update(two_triangles());
    mesh.draw();
    EXPECT_EQ(glGetError(), GLenum(GL_NO_ERROR));
}

TEST_F(TriMeshTest, MoveConstruct) {
    auto a = TriMesh::create(one_triangle());
    auto b = std::move(a);
    glGetError();
    b.draw();
    EXPECT_EQ(glGetError(), GLenum(GL_NO_ERROR));
}
