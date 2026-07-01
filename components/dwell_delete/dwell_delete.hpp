// Copyright 2026 Travis West
#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string>
#include <string_view>

#include "editor_layout.hpp"
#include "sygaldry_endpoints.hpp"

// dwell_delete (vr_editor_decomposition.md S3, from update_dwell): with grip
// held, a 1 s dwell of the tip inside a card (or near an edge midpoint) emits a
// `remove_node` / `remove_edge` op. Deliberate by design — grip + dwell, never
// bare hover. A resource-holder: it observes the graph it lives in.
struct DwellDeleteNode {
    static consteval std::string_view name() { return "dwell_delete"; }
    static consteval std::string_view source_header() {
        return "components/dwell_delete/dwell_delete.hpp";
    }
    static constexpr int lift_kind() { return lift::resource_holder; }

    struct endpoints {
        ::in<Eigen::Vector3f> pos;
        ::in<Eigen::Quaternionf> rot;
        ::in<float> grip;
    } endpoints;

    void operator()(double time_s);
    void set_context(const editor_layout::GestureContext& ctx) { ctx_ = ctx; }

private:
    editor_layout::GestureContext ctx_;
    double prev_t_ = -1.0;
    float dwell_s_ = 0.f;
    std::string dwell_card_;  // empty ⇒ none
    int dwell_edge_ = -1;
};
