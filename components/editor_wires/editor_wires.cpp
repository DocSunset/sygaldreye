// Copyright 2026 Travis West
#include "editor_wires.hpp"

namespace {
// Wire colour by producer port kind (mirrors port_kind_color for wires).
void kind_rgba(const std::string& kind, float* c) {
    if (kind == "scalar") {
        c[0] = 0.2f;
        c[1] = 0.4f;
        c[2] = 1.0f;
    } else if (kind == "vec3") {
        c[0] = 0.2f;
        c[1] = 0.9f;
        c[2] = 0.3f;
    } else if (kind == "vec4") {
        c[0] = 0.2f;
        c[1] = 0.9f;
        c[2] = 0.9f;
    } else if (kind == "audio") {
        c[0] = 1.0f;
        c[1] = 0.9f;
        c[2] = 0.1f;
    } else if (kind == "texture") {
        c[0] = 1.0f;
        c[1] = 0.5f;
        c[2] = 0.1f;
    } else {
        c[0] = 0.5f;
        c[1] = 0.5f;
        c[2] = 0.5f;
    }
    c[3] = 1.0f;
}
}  // namespace

void EditorWiresNode::operator()(double) {
    rows_.clear();
    if (!ctx_.graph) {
        endpoints.wires.value = Span{nullptr, 0, 10};
        return;
    }
    const editor_layout::Layout& l = editor_layout::cached_layout(ctx_);

    for (const auto& e : ctx_.graph->edges) {
        Eigen::Vector3f from, to;
        if (!editor_layout::edge_endpoints(l, e, from, to)) continue;
        // The producer port's kind drives the colour.
        std::string kind;
        for (const auto& c : l.cards)
            if (c.node_id == e.from_node)
                for (const auto& h : c.outputs)
                    if (h.port_name == e.from_port) kind = h.port_kind;
        float rgba[4];
        kind_rgba(kind, rgba);
        rows_.insert(
            rows_.end(),
            {from.x(),
             from.y(),
             from.z(),
             to.x(),
             to.y(),
             to.z(),
             rgba[0],
             rgba[1],
             rgba[2],
             rgba[3]});
    }
    endpoints.wires.value =
        Span{rows_.empty() ? nullptr : rows_.data(), int(rows_.size() / 10), 10};
}
