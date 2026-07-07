// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string_view>

// A hand/controller as a graph source node. On host the values are set over
// HTTP (/param or /controller); on device an XR pump feeds the same inputs.
// Consumers (the editor, grab targets) see identical ports either way.
// v6 note: param inputs that share a name with an output gained an _in
// suffix (trigger_in...); the /controller sugar maps to them, so the HTTP
// contract is unchanged. Graph edges read the outputs, names unchanged.
struct HandNode {
    static consteval std::string_view name() { return "hand"; }
    static consteval std::string_view source_header() { return "components/hand_node/hand_node.hpp"; }

    struct endpoints {
        normalled_in<float, fp(-5.f), fp(5.f), fp(0.f)>   x;
        normalled_in<float, fp(-5.f), fp(5.f), fp(1.2f)>  y;
        normalled_in<float, fp(-5.f), fp(5.f), fp(-0.4f)> z;
        normalled_in<float, fp(-1.f), fp(1.f), fp(0.f)>   qx, qy, qz;
        normalled_in<float, fp(-1.f), fp(1.f), fp(1.f)>   qw;
        normalled_in<float, fp(0.f),  fp(1.f), fp(0.f)>   trigger_in;
        normalled_in<float, fp(0.f),  fp(1.f), fp(0.f)>   grip_in;
        normalled_in<float, fp(-1.f), fp(1.f), fp(0.f)>   thumb_x_in, thumb_y_in;
        ::out<Eigen::Vector3f>    pos;
        ::out<Eigen::Quaternionf> rot;
        ::out<float> trigger, grip, thumb_x, thumb_y;
    } endpoints;

    void operator()(double) {
        endpoints.pos.value = {endpoints.x.get(), endpoints.y.get(), endpoints.z.get()};
        Eigen::Quaternionf q{endpoints.qw.get(), endpoints.qx.get(),
                             endpoints.qy.get(), endpoints.qz.get()};
        endpoints.rot.value     = (q.norm() > 1e-6f) ? q.normalized()
                                                     : Eigen::Quaternionf::Identity();
        endpoints.trigger.value = endpoints.trigger_in.get();
        endpoints.grip.value    = endpoints.grip_in.get();
        endpoints.thumb_x.value = endpoints.thumb_x_in.get();
        endpoints.thumb_y.value = endpoints.thumb_y_in.get();
    }
};
