#pragma once
#include <Eigen/Core>
#include <GLES3/gl3.h>
#include <memory>
#include <string_view>
#include <vector>
#include "sygaldry_endpoints.hpp"

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
    static consteval std::string_view name()          { return "particle_system"; }
    static consteval std::string_view source_header() { return "components/particle_system/particle_system.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/particle_system/particle_system.cpp"; }

    struct inputs {
        slider<"emit rate",    "", float, fp(0.f),   fp(500.f),  fp(50.f)>  emit_rate;
        slider<"lifetime min", "", float, fp(0.1f),  fp(10.f),   fp(0.5f)>  lifetime_min;
        slider<"lifetime max", "", float, fp(0.1f),  fp(10.f),   fp(2.f)>   lifetime_max;
        slider<"size start",   "", float, fp(0.01f), fp(1.f),    fp(0.05f)> size_start;
        slider<"size end",     "", float, fp(0.f),   fp(1.f),    fp(0.f)>   size_end;
        slider<"emit_x",       "", float, fp(-50.f),  fp(50.f),   fp(0.f)>   emit_x;
        slider<"emit_y",       "", float, fp(-50.f),  fp(50.f),   fp(0.f)>   emit_y;
        slider<"emit_z",       "", float, fp(-50.f),  fp(50.f),   fp(0.f)>   emit_z;
        slider<"vel_up",       "", float, fp(0.f),    fp(20.f),   fp(3.f)>   vel_up;
        slider<"vel_spread",   "", float, fp(0.f),    fp(10.f),   fp(1.f)>   vel_spread;
        slider<"gravity_y",    "", float, fp(-30.f),  fp(30.f),   fp(-9.8f)> gravity_y;
        slider<"r",            "", float, fp(0.f),    fp(1.f),    fp(1.f)>   r;
        slider<"g",            "", float, fp(0.f),    fp(1.f),    fp(0.75f)> g;
        slider<"b",            "", float, fp(0.f),    fp(1.f),    fp(0.3f)>  b;
    } inputs;

    explicit ParticleSystem(int capacity = 1000);
    ~ParticleSystem();
    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;
    ParticleSystem(ParticleSystem&&) noexcept;
    ParticleSystem& operator=(ParticleSystem&&) noexcept;

    struct outputs {
        port<"render", DrawFn> render;
    } outputs;

    void set_emitter(EmitterParams const&);
    void update(float dt, Eigen::Vector3f gravity = {0.f, -9.8f, 0.f});
    void operator()(double time_s);
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
    double                prev_time_ = -1.0;
};
