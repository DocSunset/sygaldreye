#pragma once
#include <memory>
#include <string_view>
#include <vector>

#include <Eigen/Core>

#include "render_payloads.hpp"  // Surface, Mesh, Shader, InstanceAttr
#include "sygaldry_endpoints.hpp"

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
    float           size_start, size_end;
    float           lifetime_min, lifetime_max;
    float           emit_rate;
};

// particle_system: the pool simulation stays a node; the DRAW leaves. Each
// tick it emits a Mesh (a unit quad + per-instance aOffset/aSize/aColor Spans
// from the live pool) + a camera-facing, blended particle Surface. The shared
// instanced-draw strategy in render_region does the rest — no GL here.
class ParticleSystem {
   public:
    static consteval std::string_view name() { return "particle_system"; }
    static consteval std::string_view source_header() {
        return "components/particle_system/particle_system.hpp";
    }
    static consteval std::string_view source_cpp() {
        return "components/particle_system/particle_system.cpp";
    }

    struct endpoints {
        normalled_in<float, fp(0.f), fp(500.f), fp(50.f)> emit_rate;
        normalled_in<float, fp(0.1f), fp(10.f), fp(0.5f)> lifetime_min;
        normalled_in<float, fp(0.1f), fp(10.f), fp(2.f)> lifetime_max;
        normalled_in<float, fp(0.01f), fp(1.f), fp(0.05f)> size_start;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> size_end;
        normalled_in<float, fp(-50.f), fp(50.f), fp(0.f)> emit_x;
        normalled_in<float, fp(-50.f), fp(50.f), fp(0.f)> emit_y;
        normalled_in<float, fp(-50.f), fp(50.f), fp(0.f)> emit_z;
        normalled_in<float, fp(0.f), fp(20.f), fp(3.f)> vel_up;
        normalled_in<float, fp(0.f), fp(10.f), fp(1.f)> vel_spread;
        normalled_in<float, fp(-30.f), fp(30.f), fp(-9.8f)> gravity_y;
        normalled_in<float, fp(0.f), fp(1.f), fp(1.f)> r;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.75f)> g;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.3f)> b;
        ::out<Surface> surface;
        ::out<Mesh>    mesh;
    } endpoints;

    explicit ParticleSystem(int capacity = 1000);

    void set_emitter(EmitterParams const&);
    void update(float dt, Eigen::Vector3f gravity = {0.f, -9.8f, 0.f});
    void operator()(double time_s);

   private:
    void emit_one(float t);

    std::vector<Particle> pool_;
    int                   capacity_;
    int                   next_dead_  = 0;
    float                 emit_accum_ = 0.f;
    EmitterParams         params_{};
    double                prev_time_ = -1.0;

    // Per-instance attribute buffers (packed from live particles each frame).
    std::vector<float> positions_, sizes_, colors_;
    MeshPtr            quad_;
    Shader             shader_;
};
