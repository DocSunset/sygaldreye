// Copyright 2025 Travis West
#pragma once
#include <openxr/openxr.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string_view>
#include "sygaldry_endpoints.hpp"

struct HeadPoseNode {
    static consteval std::string_view name()          { return "head_pose"; }
    static consteval std::string_view source_header() { return "components/xr_sources/xr_sources.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/xr_sources/xr_sources.cpp"; }

    struct endpoints {
        out<float> pos_x, pos_y, pos_z;
        out<float> rot_x, rot_y, rot_z, rot_w;
    } endpoints;

    void set_pose(const XrPosef& p) {
        pose_ = p;
        endpoints.pos_x.value = p.position.x;
        endpoints.pos_y.value = p.position.y;
        endpoints.pos_z.value = p.position.z;
        endpoints.rot_x.value = p.orientation.x;
        endpoints.rot_y.value = p.orientation.y;
        endpoints.rot_z.value = p.orientation.z;
        endpoints.rot_w.value = p.orientation.w;
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

    struct endpoints {
        out<float> pos_x, pos_y, pos_z;
        out<float> rot_x, rot_y, rot_z, rot_w;
        out<float> trigger, grip;
        out<float> thumbstick_x, thumbstick_y;
        out<float> btn1, btn2;   // X/A, Y/B
        out<Eigen::Vector3f>    pos;  // parity with host 'hand'
        out<Eigen::Quaternionf> rot;
    } endpoints;

    void set_state(const XrPosef& p, bool trigger_pressed,
                   float grip = 0.f,
                   float thumb_x = 0.f, float thumb_y = 0.f,
                   bool btn1 = false, bool btn2 = false) {
        pose_    = p;
        trigger_ = trigger_pressed;
        endpoints.pos_x.value        = p.position.x;
        endpoints.pos_y.value        = p.position.y;
        endpoints.pos_z.value        = p.position.z;
        endpoints.rot_x.value        = p.orientation.x;
        endpoints.rot_y.value        = p.orientation.y;
        endpoints.rot_z.value        = p.orientation.z;
        endpoints.rot_w.value        = p.orientation.w;
        endpoints.trigger.value      = trigger_pressed ? 1.f : 0.f;
        endpoints.grip.value         = grip;
        endpoints.thumbstick_x.value = thumb_x;
        endpoints.thumbstick_y.value = thumb_y;
        endpoints.btn1.value         = btn1 ? 1.f : 0.f;
        endpoints.btn2.value         = btn2 ? 1.f : 0.f;
        endpoints.pos.value          = {p.position.x, p.position.y, p.position.z};
        endpoints.rot.value          = Eigen::Quaternionf{p.orientation.w, p.orientation.x,
                                                          p.orientation.y, p.orientation.z};
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
