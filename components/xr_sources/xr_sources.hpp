// Copyright 2025 Travis West
#pragma once
#include <openxr/openxr.h>
#include <string_view>
#include "sygaldry_endpoints.hpp"

struct HeadPoseNode {
    static consteval std::string_view name()          { return "head_pose"; }
    static consteval std::string_view source_header() { return "components/xr_sources/xr_sources.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/xr_sources/xr_sources.cpp"; }

    struct inputs {} inputs;

    struct outputs {
        port<"pos_x", float> pos_x;
        port<"pos_y", float> pos_y;
        port<"pos_z", float> pos_z;
        port<"rot_x", float> rot_x;
        port<"rot_y", float> rot_y;
        port<"rot_z", float> rot_z;
        port<"rot_w", float> rot_w;
    } outputs;

    void set_pose(const XrPosef& p) {
        pose_ = p;
        outputs.pos_x.value = p.position.x;
        outputs.pos_y.value = p.position.y;
        outputs.pos_z.value = p.position.z;
        outputs.rot_x.value = p.orientation.x;
        outputs.rot_y.value = p.orientation.y;
        outputs.rot_z.value = p.orientation.z;
        outputs.rot_w.value = p.orientation.w;
    }

    XrPosef current_pose() const { return pose_; }
    void operator()(double) {}

private:
    XrPosef pose_{};
};

template<fixed_string NodeName>
struct ControllerNode {
    static consteval std::string_view name()          { return NodeName; }
    static consteval std::string_view source_header() { return "components/xr_sources/xr_sources.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/xr_sources/xr_sources.cpp"; }

    struct inputs {} inputs;

    struct outputs {
        port<"pos_x",        float> pos_x;
        port<"pos_y",        float> pos_y;
        port<"pos_z",        float> pos_z;
        port<"rot_x",        float> rot_x;
        port<"rot_y",        float> rot_y;
        port<"rot_z",        float> rot_z;
        port<"rot_w",        float> rot_w;
        port<"trigger",      float> trigger;
        port<"grip",         float> grip;
        port<"thumbstick_x", float> thumbstick_x;
        port<"thumbstick_y", float> thumbstick_y;
    } outputs;

    void set_state(const XrPosef& p, bool trigger_pressed,
                   float grip = 0.f,
                   float thumb_x = 0.f, float thumb_y = 0.f) {
        pose_    = p;
        trigger_ = trigger_pressed;
        outputs.pos_x.value        = p.position.x;
        outputs.pos_y.value        = p.position.y;
        outputs.pos_z.value        = p.position.z;
        outputs.rot_x.value        = p.orientation.x;
        outputs.rot_y.value        = p.orientation.y;
        outputs.rot_z.value        = p.orientation.z;
        outputs.rot_w.value        = p.orientation.w;
        outputs.trigger.value      = trigger_pressed ? 1.f : 0.f;
        outputs.grip.value         = grip;
        outputs.thumbstick_x.value = thumb_x;
        outputs.thumbstick_y.value = thumb_y;
    }

    XrPosef pose() const { return pose_; }
    bool trigger_pressed() const { return trigger_; }
    void operator()(double) {}

private:
    XrPosef pose_{};
    bool    trigger_ = false;
};

using LeftControllerNode  = ControllerNode<"left_controller">;
using RightControllerNode = ControllerNode<"right_controller">;
