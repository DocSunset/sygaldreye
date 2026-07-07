// Copyright 2026 Travis West
#pragma once
#include "fly_camera.hpp"
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <string_view>

// The camera as a graph node: anything can patch into its pose; the platform
// reads the pv output to render. aspect is pumped by the platform each frame.
// v6 note: yaw/pitch outputs gained an _out suffix (the params keep the
// short names — they are the write contract for /camera and orbit rigs).
struct FlyCameraNode {
    static consteval std::string_view name() { return "fly_camera"; }
    static consteval std::string_view source_header() { return "components/fly_camera_node/fly_camera_node.hpp"; }

    struct endpoints {
        normalled_in<float, fp(-200.f),  fp(200.f),   fp(0.f)>    x;
        normalled_in<float, fp(-200.f),  fp(200.f),   fp(2.f)>    y;
        normalled_in<float, fp(-200.f),  fp(200.f),   fp(8.f)>    z;
        normalled_in<float, fp(-6.2832f),fp(6.2832f), fp(0.f)>    yaw;
        normalled_in<float, fp(-1.55f),  fp(1.55f),   fp(0.f)>    pitch;
        normalled_in<float, fp(0.2f),    fp(2.5f),    fp(0.9f)>   fov;
        normalled_in<float, fp(0.1f),    fp(5.f),     fp(1.777f)> aspect;
        ::out<Eigen::Matrix4f> view, proj, pv;
        ::out<Eigen::Vector3f> pos;
        ::out<float> yaw_out, pitch_out;
    } endpoints;

    void operator()(double) {
        FlyCamera c{{endpoints.x.get(), endpoints.y.get(), endpoints.z.get()},
                    endpoints.yaw.get(), endpoints.pitch.get(), endpoints.fov.get()};
        endpoints.view.value      = c.view();
        endpoints.proj.value      = c.proj(endpoints.aspect.get());
        endpoints.pv.value        = endpoints.proj.value * endpoints.view.value;
        endpoints.pos.value       = c.pos;
        endpoints.yaw_out.value   = c.yaw;
        endpoints.pitch_out.value = c.pitch;
    }
};
