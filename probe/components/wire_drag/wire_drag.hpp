// Copyright 2026 Travis West
#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string>
#include <string_view>

#include "editor_layout.hpp"
#include "sygaldry_endpoints.hpp"

// wire_drag (vr_editor_decomposition.md S3, from update_drag): the grip-edge
// FSM. On grip-down, grab the nearest OUTPUT handle under the tip; on grip-up,
// release on the nearest kind-compatible INPUT handle → emit an `add_edge` op.
// No output handle under the tip ⇒ grab the card BODY and move it while gripped
// (writes graph_source's shared position overrides → the card follows). A
// resource-holder: it observes the graph it lives in.
struct WireDragNode {
    static consteval std::string_view name() { return "wire_drag"; }
    static consteval std::string_view source_header() {
        return "components/wire_drag/wire_drag.hpp";
    }
    static constexpr int lift_kind() { return lift::resource_holder; }

    struct endpoints {
        ::in<Eigen::Vector3f> pos;     // right controller position
        ::in<Eigen::Quaternionf> rot;  // right controller orientation
        ::in<float> grip;              // right grip (0/1)
    } endpoints;

    void operator()(double);
    void set_context(const editor_layout::GestureContext& ctx) { ctx_ = ctx; }

private:
    enum class State { Idle, Dragging, MovingCard };
    editor_layout::GestureContext ctx_;
    State state_ = State::Idle;
    bool prev_grip_ = false;
    std::string from_node_, from_port_, from_kind_;
    std::string move_id_;
    Eigen::Vector3f move_offset_{0, 0, 0};
};
