// Copyright 2026 Travis West
#pragma once
#include "vr_editor.hpp"
#include "text_mesh.hpp"
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <optional>
#include <string>
#include <string_view>

// The editor as a node in the graph it edits. Hand poses arrive through
// ordinary edges (from hand/controller source nodes), so the editor is
// source-agnostic: virtual hands on host, XR controllers on device.
// migrate_graph keeps this instance (and its undo state) alive across the
// very edits it produces.
//
// Context seam: the platform must call set_context() each frame before
// tick (the enclosing graph can't be a port — it contains this node), and
// collect take_edit() after.
struct EditorNode {
    static consteval std::string_view name() { return "editor"; }
    static consteval std::string_view source_header() { return "components/editor_node/editor_node.hpp"; }

    struct inputs {
        port<"left_pos",  Eigen::Vector3f>    left_pos;
        port<"left_rot",  Eigen::Quaternionf> left_rot;
        port<"right_pos", Eigen::Vector3f>    right_pos;
        port<"right_rot", Eigen::Quaternionf> right_rot;
        slider<"trigger_left",  "", float, fp(0.f),  fp(1.f), fp(0.f)> trigger_left;
        slider<"trigger_right", "", float, fp(0.f),  fp(1.f), fp(0.f)> trigger_right;
        slider<"grip_right",    "", float, fp(0.f),  fp(1.f), fp(0.f)> grip_right;
        slider<"thumb_x",       "", float, fp(-1.f), fp(1.f), fp(0.f)> thumb_x;
        slider<"thumb_y",       "", float, fp(-1.f), fp(1.f), fp(0.f)> thumb_y;
    } inputs;

    struct outputs {
        port<"render", DrawFn> render;
    } outputs;

    void operator()(double time_s);

    void set_context(const Graph* g, const ComponentRegistry* r) {
        graph_ = g; registry_ = r;
    }
    std::optional<std::string> take_edit() {
        if (pending_edit_.empty()) return std::nullopt;
        std::string out = std::move(pending_edit_);
        pending_edit_.clear();
        return out;
    }

private:
    VrEditor    editor_;
    TextMesh    text_;
    bool        ready_ = false;
    const Graph*             graph_    = nullptr;
    const ComponentRegistry* registry_ = nullptr;
    std::string pending_edit_;
    double      prev_time_s_ = 0.0;
};
