// Copyright 2025 Travis West
#pragma once
#include "gl_program.hpp"
#include "sygaldry_endpoints.hpp"
#include "visual_node.hpp"
#include <Eigen/Core>
#include <memory>
#include <string_view>
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

class Aurora : public VisualNode<Aurora> {
public:
    static consteval std::string_view name()          { return "aurora"; }
    static consteval std::string_view source_header() { return "components/aurora/aurora.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/aurora/aurora.cpp"; }

    struct inputs {
        slider<"curtain_width",    "", float, fp(50.f),  fp(1000.f), fp(400.f)> curtain_width;
        slider<"altitude_base",    "", float, fp(0.f),   fp(200.f),  fp(60.f)>  altitude_base;
        slider<"altitude_height",  "", float, fp(10.f),  fp(500.f),  fp(170.f)> altitude_height;
        slider<"ripple_amplitude", "", float, fp(0.f),   fp(100.f),  fp(22.f)>  ripple_amplitude;
        slider<"ripple_speed",     "", float, fp(0.f),   fp(5.f),    fp(0.5f)>  ripple_speed;
    } inputs;

    struct outputs {
        port<"render", DrawFn> render;
    } outputs;

    static Aurora create(AuroraParams const&);
    bool gl_ready() const { return prog_ != nullptr; }
    void init_gl();
    void sync_params();
    void update(float time_s);
    void draw(Eigen::Matrix4f const& vp) const;

    ~Aurora();
    Aurora();
    Aurora(Aurora const&) = delete;
    Aurora& operator=(Aurora const&) = delete;
    Aurora(Aurora&&) noexcept;
    Aurora& operator=(Aurora&&) noexcept;

private:
    struct RawTag {};
    explicit Aurora(RawTag) {}
    static Aurora create_default();

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
