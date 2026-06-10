// Copyright 2026 Travis West
#pragma once
#include "fly_camera.hpp"
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <string_view>

// The camera as a graph node: anything can patch into its pose; the platform
// reads outputs.pv to render. aspect is pumped by the platform each frame.
struct FlyCameraNode {
    static consteval std::string_view name() { return "fly_camera"; }
    static consteval std::string_view source_header() { return "components/fly_camera_node/fly_camera_node.hpp"; }

    struct inputs {
        slider<"x",      "", float, fp(-200.f),  fp(200.f),   fp(0.f)>    x;
        slider<"y",      "", float, fp(-200.f),  fp(200.f),   fp(2.f)>    y;
        slider<"z",      "", float, fp(-200.f),  fp(200.f),   fp(8.f)>    z;
        slider<"yaw",    "", float, fp(-6.2832f),fp(6.2832f), fp(0.f)>    yaw;
        slider<"pitch",  "", float, fp(-1.55f),  fp(1.55f),   fp(0.f)>    pitch;
        slider<"fov",    "", float, fp(0.2f),    fp(2.5f),    fp(0.9f)>   fov;
        slider<"aspect", "", float, fp(0.1f),    fp(5.f),     fp(1.777f)> aspect;
    } inputs;

    struct outputs {
        port<"view",  Eigen::Matrix4f> view;
        port<"proj",  Eigen::Matrix4f> proj;
        port<"pv",    Eigen::Matrix4f> pv;
        port<"pos",   Eigen::Vector3f> pos;
        port<"yaw",   float>           yaw;
        port<"pitch", float>           pitch;
    } outputs;

    void operator()(double) {
        FlyCamera c{{inputs.x.value, inputs.y.value, inputs.z.value},
                    inputs.yaw.value, inputs.pitch.value, inputs.fov.value};
        outputs.view.value  = c.view();
        outputs.proj.value  = c.proj(inputs.aspect.value);
        outputs.pv.value    = outputs.proj.value * outputs.view.value;
        outputs.pos.value   = c.pos;
        outputs.yaw.value   = c.yaw;
        outputs.pitch.value = c.pitch;
    }
};
