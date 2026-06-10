// Copyright 2025 Travis West
#pragma once
#include "gl_program.hpp"
#include <Eigen/Core>
#include <memory>
#include <vector>

struct AuroraParams {
    int   curtain_count      = 7;
    int   curtain_segments_w = 60;
    int   curtain_segments_h = 24;
    float curtain_width      = 400.f;  // depth extent (Z span)
    float altitude_base      = 60.f;   // bottom of curtains
    float altitude_height    = 170.f;  // height of each curtain
    float ripple_amplitude   = 22.f;
    float ripple_speed       = 0.5f;
};

class Aurora {
public:
    static Aurora create(AuroraParams const&);
    void update(float time_s);
    void draw(Eigen::Matrix4f const& vp) const;

    ~Aurora();
    Aurora() = default;
    Aurora(Aurora const&) = delete;
    Aurora& operator=(Aurora const&) = delete;
    Aurora(Aurora&&) noexcept;
    Aurora& operator=(Aurora&&) noexcept;

private:
    struct Curtain {
        GLuint vao          = 0;
        GLuint vbo          = 0;
        GLsizei index_count = 0;
        Eigen::Vector3f color;
        float x_offset = 0.f;
        float z_offset = 0.f;
        float phase    = 0.f;
        float freq     = 1.f;
        float speed    = 1.f;
    };

    AuroraParams params_;
    std::unique_ptr<GlProgram> prog_;
    std::vector<Curtain> curtains_;
    GLuint ebo_           = 0;
    GLsizei index_count_  = 0;
    float   time_s_       = 0.f;

    GLint vp_loc_          = -1;
    GLint time_loc_        = -1;
    GLint color_loc_       = -1;
    GLint x_offset_loc_    = -1;
    GLint phase_loc_       = -1;
    GLint freq_loc_        = -1;
    GLint speed_loc_       = -1;
    GLint ramp_amp_loc_    = -1;
    GLint alt_base_loc_    = -1;
    GLint alt_height_loc_  = -1;
    GLint depth_loc_       = -1;
};
