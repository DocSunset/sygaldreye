// Copyright 2025 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include "text_mesh.hpp"
#include <Eigen/Core>
#include <string>
#include <string_view>

class TextLabelNode {
public:
    static consteval std::string_view name()          { return "text_label"; }
    static consteval std::string_view source_header() { return "components/text_label/text_label.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/text_label/text_label.cpp"; }

    struct endpoints {
        normalled_in<float, fp(-50.f), fp(50.f), fp(0.f)>  pos_x;
        normalled_in<float, fp(-50.f), fp(50.f), fp(1.5f)> pos_y;
        normalled_in<float, fp(-50.f), fp(50.f), fp(-2.f)> pos_z;
        normalled_in<float, fp(0.01f), fp(5.f),  fp(0.3f)> scale;
        normalled_in<std::string> text;   // text EDGE target (stt.text → here)
        ::out<DrawFn> render;
    } endpoints;

    void operator()(double time_s);

private:
    TextMesh mesh_;
    bool     initialized_ = false;
};
