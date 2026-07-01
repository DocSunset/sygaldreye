// Copyright 2026 Travis West
#include "handle_picker.hpp"

#include <cmath>
#include <cstdio>

using editor_layout::Layout;

void HandlePickerNode::operator()(double) {
    endpoints.label.value.clear();
    if (!ctx_.graph) return;

    Eigen::Vector3f tip = editor_layout::controller_tip(endpoints.pos.get(), endpoints.rot.get());
    Layout l = editor_layout::build_layout(
        *ctx_.graph, ctx_.overrides ? *ctx_.overrides : editor_layout::PosOverrides{});

    // Nearest handle within 0.05 m; an actively-near slider overrides with a
    // value readout (vr_editor.cpp hover loop).
    float best = 0.05f;
    for (const auto& c : l.cards) {
        for (const auto& h : c.inputs) {
            float d = (tip - h.world_pos).norm();
            if (d < best) {
                best = d;
                endpoints.label.value = h.port_name + " ◂";
                endpoints.pos_out.value = h.world_pos;
            }
        }
        for (const auto& h : c.outputs) {
            float d = (tip - h.world_pos).norm();
            if (d < best) {
                best = d;
                endpoints.label.value = "▸ " + h.port_name;
                endpoints.pos_out.value = h.world_pos;
            }
        }
        for (const auto& sl : c.sliders) {
            Eigen::Vector3f d = tip - sl.world_pos;
            if (std::abs(d.x()) < sl.width * 0.5f && std::abs(d.y()) < 0.03f &&
                std::abs(d.z()) < 0.06f) {
                char buf[96];
                std::snprintf(
                    buf, sizeof(buf), "%s = %.3g", sl.port_name.c_str(), double(sl.value));
                endpoints.label.value = buf;
                endpoints.pos_out.value = sl.world_pos;
            }
        }
    }
}
