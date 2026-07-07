// Copyright 2025 Travis West
#pragma once
#include <memory>
#include <string_view>

#include "render_payloads.hpp"  // Surface, Mesh, Shader
#include "sygaldry_endpoints.hpp"

// text_label: lays out an MSDF string into a dynamic quad Mesh (UV in the
// unused normal.xy) + a Surface that samples the glyph atlas (uploaded by
// render_region as a CPU image). GL lives in render_region.
class TextLabelNode {
   public:
    static consteval std::string_view name() { return "text_label"; }
    static consteval std::string_view source_header() {
        return "components/text_label/text_label.hpp";
    }
    static consteval std::string_view source_cpp() { return "components/text_label/text_label.cpp"; }

    struct endpoints {
        normalled_in<float, fp(-50.f), fp(50.f), fp(0.f)> pos_x;
        normalled_in<float, fp(-50.f), fp(50.f), fp(1.5f)> pos_y;
        normalled_in<float, fp(-50.f), fp(50.f), fp(-2.f)> pos_z;
        normalled_in<float, fp(0.01f), fp(5.f), fp(0.3f)> scale;
        normalled_in<std::string> text;  // text EDGE target (stt.text → here)
        ::out<Surface>            surface;
        ::out<Mesh>               mesh;
    } endpoints;

    void operator()(double time_s);

   private:
    Shader shader_;
};
