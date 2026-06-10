// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string_view>

// A hand/controller as a graph source node. On host the values are set over
// HTTP (/param or /controller); on device an XR pump feeds the same inputs.
// Consumers (the editor, grab targets) see identical ports either way.
struct HandNode {
    static consteval std::string_view name() { return "hand"; }
    static consteval std::string_view source_header() { return "components/hand_node/hand_node.hpp"; }

    struct inputs {
        slider<"x",       "", float, fp(-5.f), fp(5.f), fp(0.f)>  x;
        slider<"y",       "", float, fp(-5.f), fp(5.f), fp(1.2f)> y;
        slider<"z",       "", float, fp(-5.f), fp(5.f), fp(-0.4f)> z;
        slider<"qx",      "", float, fp(-1.f), fp(1.f), fp(0.f)>  qx;
        slider<"qy",      "", float, fp(-1.f), fp(1.f), fp(0.f)>  qy;
        slider<"qz",      "", float, fp(-1.f), fp(1.f), fp(0.f)>  qz;
        slider<"qw",      "", float, fp(-1.f), fp(1.f), fp(1.f)>  qw;
        slider<"trigger", "", float, fp(0.f),  fp(1.f), fp(0.f)>  trigger;
        slider<"grip",    "", float, fp(0.f),  fp(1.f), fp(0.f)>  grip;
        slider<"thumb_x", "", float, fp(-1.f), fp(1.f), fp(0.f)>  thumb_x;
        slider<"thumb_y", "", float, fp(-1.f), fp(1.f), fp(0.f)>  thumb_y;
    } inputs;

    struct outputs {
        port<"pos",     Eigen::Vector3f>    pos;
        port<"rot",     Eigen::Quaternionf> rot;
        port<"trigger", float>              trigger;
        port<"grip",    float>              grip;
        port<"thumb_x", float>              thumb_x;
        port<"thumb_y", float>              thumb_y;
    } outputs;

    void operator()(double) {
        outputs.pos.value = {inputs.x.value, inputs.y.value, inputs.z.value};
        Eigen::Quaternionf q{inputs.qw.value, inputs.qx.value,
                             inputs.qy.value, inputs.qz.value};
        outputs.rot.value     = (q.norm() > 1e-6f) ? q.normalized()
                                                   : Eigen::Quaternionf::Identity();
        outputs.trigger.value = inputs.trigger.value;
        outputs.grip.value    = inputs.grip.value;
        outputs.thumb_x.value = inputs.thumb_x.value;
        outputs.thumb_y.value = inputs.thumb_y.value;
    }
};
