// Copyright 2025 Travis West
#pragma once
#include "gl_program.hpp"
#include <Eigen/Core>
#include <memory>
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
    static Lissajous create(LissajousParams const&);
    void update(float time_s);
    void draw(Eigen::Matrix4f const& mvp) const;

private:
    LissajousParams params_;
    std::vector<float> vbo_data_; // interleaved: x,y,z,r,g,b (6 floats per vertex)
    GLuint vao_  = 0;
    GLuint vbo_  = 0;
    std::unique_ptr<GlProgram> prog_;
    GLint mvp_loc_ = -1;
};
