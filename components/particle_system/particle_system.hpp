#pragma once
#include <Eigen/Core>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>

struct GlProgram;

struct Particle {
    Eigen::Vector3f position;
    Eigen::Vector3f velocity;
    Eigen::Vector4f color;
    float           size;
    float           life;
};

struct EmitterParams {
    Eigen::Vector3f origin;
    Eigen::Vector3f velocity_min, velocity_max;
    Eigen::Vector4f color_start, color_end;
    float size_start, size_end;
    float lifetime_min, lifetime_max;
    float emit_rate;
};

class ParticleSystem {
public:
    explicit ParticleSystem(int capacity);
    ~ParticleSystem();
    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;
    ParticleSystem(ParticleSystem&&) noexcept;
    ParticleSystem& operator=(ParticleSystem&&) noexcept;

    void set_emitter(EmitterParams const&);
    void update(float dt, Eigen::Vector3f gravity = {0.f, -9.8f, 0.f});
    void draw(Eigen::Matrix4f const& vp,
              Eigen::Vector3f camera_right,
              Eigen::Vector3f camera_up) const;

private:
    void create_gl_resources();
    void destroy_gl_resources();
    void emit_one(float t);

    std::vector<Particle> pool_;
    int                   capacity_;
    int                   next_dead_  = 0;
    float                 emit_accum_ = 0.f;
    EmitterParams         params_{};

    std::unique_ptr<GlProgram> prog_;
    GLuint vao_      = 0;
    GLuint quad_vbo_ = 0;
    GLuint inst_vbo_ = 0;
    GLint  loc_vp_    = -1;
    GLint  loc_right_ = -1;
    GLint  loc_up_    = -1;

    mutable std::vector<float> inst_buf_;
};
