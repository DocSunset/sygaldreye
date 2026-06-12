// Copyright 2025 Travis West
#pragma once
#include <Eigen/Core>
#include <GLES3/gl3.h>
#include <memory>
#include <string_view>
#include "sygaldry_endpoints.hpp"

struct GlProgram;

class StarField {
public:
    static consteval std::string_view name()          { return "star_field"; }
    static consteval std::string_view source_header() { return "components/star_field/star_field.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/star_field/star_field.cpp"; }

    struct endpoints {
        normalled_in<float, fp(-1.f), fp(1.f), fp(0.5f)> sun_elevation;
        normalled_in<float, fp(0.f), fp(5000.f), fp(2000.f)> star_count;
        normalled_in<float, fp(10.f), fp(2000.f), fp(500.f)> radius;
    
        ::out<DrawFn> render;
    } endpoints;


    static StarField create(int star_count, float radius);
    void operator()(double);
    void draw(Eigen::Matrix4f const& vp) const;

    StarField() = default;
    ~StarField();
    StarField(const StarField&) = delete;
    StarField& operator=(const StarField&) = delete;
    StarField(StarField&&) noexcept;
    StarField& operator=(StarField&&) noexcept;

private:
    std::unique_ptr<GlProgram> prog_;
    GLuint  vao_         = 0;
    GLint   vp_loc_      = -1;
    GLint   alpha_loc_   = -1;
    GLint   radius_loc_  = -1;
    GLsizei star_count_  = 0;
    float   radius_      = 500.f;
    float   sun_elev_    = 0.5f;
};
