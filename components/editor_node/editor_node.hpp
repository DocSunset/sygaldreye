// Copyright 2026 Travis West
#pragma once
#include "vr_editor.hpp"
#include "text_mesh.hpp"
#include "sygaldry_endpoints.hpp"
#include "event_queue.hpp"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string>
#include <string_view>

// The editor as a node in the graph it edits. Hand poses arrive through
// ordinary edges (from hand/controller source nodes), so the editor is
// source-agnostic: virtual hands on host, XR controllers on device.
// migrate_graph keeps this instance (and its undo state) alive across the
// very edits it produces.
//
// Context seam: the platform must call set_context() each frame before
// tick (the enclosing graph can't be a port — it contains this node).
// Edits flow out through the queue mapping: never dropped, drained by the
// shell at its own cadence.
struct EditorNode {
    static consteval std::string_view name() { return "editor"; }
    static consteval std::string_view source_header() { return "components/editor_node/editor_node.hpp"; }

    struct endpoints {
        in<Eigen::Vector3f>    left_pos;
        in<Eigen::Quaternionf> left_rot;
        in<Eigen::Vector3f>    right_pos;
        in<Eigen::Quaternionf> right_rot;
        normalled_in<float, fp(0.f),  fp(1.f), fp(0.f)> trigger_left;
        normalled_in<float, fp(0.f),  fp(1.f), fp(0.f)> trigger_right;
        normalled_in<float, fp(0.f),  fp(1.f), fp(0.f)> grip_right;
        normalled_in<float, fp(-1.f), fp(1.f), fp(0.f)> thumb_x;
        normalled_in<float, fp(-1.f), fp(1.f), fp(0.f)> thumb_y;
        normalled_in<float, fp(0.1f), fp(2.f), fp(0.5f)> text_scale;
        out<DrawFn> render;
    } endpoints;

    void operator()(double time_s);

    void set_context(const Graph* g, const ComponentRegistry* r,
                     EventQueue<std::string>* edits) {
        graph_ = g; registry_ = r; edits_ = edits;
    }

private:
    VrEditor    editor_;
    TextMesh    text_;
    bool        ready_ = false;
    const Graph*             graph_      = nullptr;
    const Graph*             last_graph_ = nullptr;
    const ComponentRegistry* registry_   = nullptr;
    EventQueue<std::string>* edits_      = nullptr;
    double      prev_time_s_ = 0.0;
};
