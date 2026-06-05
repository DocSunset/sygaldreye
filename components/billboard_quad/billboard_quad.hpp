#pragma once
#include <Eigen/Core>
#include <GLES3/gl3.h>
#include <memory>
#include <span>
#include <vector>

struct GlProgram;

struct BillboardInstance {
    Eigen::Vector3f position;
    Eigen::Vector2f size;
    Eigen::Vector4f color;
};

class BillboardQuad {
public:
    static BillboardQuad create(int max_instances);

    BillboardQuad() = default;
    ~BillboardQuad();
    BillboardQuad(BillboardQuad const&) = delete;
    BillboardQuad& operator=(BillboardQuad const&) = delete;
    BillboardQuad(BillboardQuad&&) noexcept;
    BillboardQuad& operator=(BillboardQuad&&) noexcept;

    void set_instances(std::span<BillboardInstance const> instances);
    void draw(Eigen::Matrix4f const& vp,
              Eigen::Vector3f camera_right,
              Eigen::Vector3f camera_up) const;

private:
    std::unique_ptr<GlProgram> prog_;
    GLuint vao_      = 0;
    GLuint quad_vbo_ = 0;
    GLuint inst_vbo_ = 0;
    GLint  loc_vp_    = -1;
    GLint  loc_right_ = -1;
    GLint  loc_up_    = -1;
    int    instance_count_ = 0;
    int    max_instances_  = 0;
    mutable std::vector<float> pack_buf_;
};
