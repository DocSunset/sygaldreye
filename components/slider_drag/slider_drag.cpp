// Copyright 2026 Travis West
#include "slider_drag.hpp"

#include <algorithm>
#include <cstdio>

using editor_layout::Layout;
using editor_layout::Slider;

void SliderDragNode::operator()(double) {
    if (!ctx_.graph || !ctx_.edits) return;
    if (endpoints.trigger.get() <= 0.5f) return;

    Eigen::Vector3f tip = editor_layout::controller_tip(endpoints.pos.get(), endpoints.rot.get());
    const Layout& l = editor_layout::cached_layout(ctx_);

    // The single nearest track to the tip (half a port row in y; within the
    // track width in x; close in z). Sweeping neighbours during a vertical
    // drag is what the y-nearest pick prevents.
    const Slider* best = nullptr;
    float best_dy = 0.009f;
    for (const auto& c : l.cards)
        for (const auto& sl : c.sliders) {
            Eigen::Vector3f d = tip - sl.world_pos;
            if (std::abs(d.x()) < sl.width * 0.5f && std::abs(d.z()) < 0.05f &&
                std::abs(d.y()) < best_dy) {
                best_dy = std::abs(d.y());
                best = &sl;
            }
        }
    if (!best) return;

    float dx = tip.x() - best->world_pos.x();
    float norm = std::clamp((dx + best->width * 0.5f) / best->width, 0.f, 1.f);
    float value = best->min_val + norm * (best->max_val - best->min_val);

    char num[32];
    std::snprintf(num, sizeof(num), "%g", double(value));
    using editor_layout::json_escape;
    ctx_.edits->push(
        "{\"op\":\"set_param\",\"id\":\"" + json_escape(best->node_id) + "\",\"port\":\"" +
        json_escape(best->port_name) + "\",\"value\":" + num + "}");
}
