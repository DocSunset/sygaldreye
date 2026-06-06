// Copyright 2025 Travis West
#pragma once
#include "gl_program.hpp"
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <memory>
#include <string_view>
#include <vector>

struct LissajousParams {
    float freq_x    = 3.0f;
    float freq_y    = 4.0f;
    float freq_z    = 0.5f;   // slow z modulation
    float phase_x   = 0.0f;   // animated over time
    float amp       = 1.0f;
    int   samples   = 4000;
};

class Lissajous {
public:
    static consteval std::string_view name()          { return "lissajous"; }
    static consteval std::string_view source_header() { return "components/lissajous/lissajous.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/lissajous/lissajous.cpp"; }

    struct inputs {
        slider<"freq x",   "", float, fp(0.5f),  fp(20.0f), fp(3.0f)>    freq_x;
        slider<"freq y",   "", float, fp(0.5f),  fp(20.0f), fp(4.0f)>    freq_y;
        slider<"freq z",   "", float, fp(0.0f),  fp(5.0f),  fp(0.5f)>    freq_z;
        slider<"phase x",  "", float, fp(0.0f),  fp(6.283f),fp(0.0f)>    phase_x;
        slider<"amp",      "", float, fp(0.1f),  fp(5.0f),  fp(1.0f)>    amp;
        slider<"samples",  "", float, fp(100.f), fp(10000.f),fp(4000.f)> samples;
    } inputs;

    struct outputs {
        port<"render", DrawFn> render;
    } outputs;

    Lissajous() { *this = create({}); }

    static Lissajous create(LissajousParams const&);
    void update(float time_s);
    void operator()(double time_s);
    void draw(Eigen::Matrix4f const& mvp) const;

private:
    LissajousParams params_;
    std::vector<float> vbo_data_; // interleaved: x,y,z,r,g,b (6 floats per vertex)
    GLuint vao_  = 0;
    GLuint vbo_  = 0;
    std::unique_ptr<GlProgram> prog_;
    GLint mvp_loc_ = -1;
};
