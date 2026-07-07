// Copyright 2026 Travis West
#include "dwell_delete.hpp"

using editor_layout::Card;
using editor_layout::Layout;

namespace {
const Card* card_under(const Layout& l, const Eigen::Vector3f& tip) {
    for (const auto& c : l.cards) {
        Eigen::Vector3f d = tip - c.position;
        if (std::abs(d.x()) < c.width * 0.5f && std::abs(d.y()) < c.height * 0.5f &&
            std::abs(d.z()) < 0.08f)
            return &c;
    }
    return nullptr;
}
}  // namespace

void DwellDeleteNode::operator()(double time_s) {
    float dt = (prev_t_ >= 0.0) ? float(time_s - prev_t_) : 0.f;
    prev_t_ = time_s;
    if (!ctx_.graph || !ctx_.edits) return;

    // Deliberate: grip held; never delete with no grip.
    if (endpoints.grip.get() <= 0.5f) {
        dwell_s_ = 0.f;
        dwell_card_.clear();
        dwell_edge_ = -1;
        return;
    }

    Eigen::Vector3f tip = editor_layout::controller_tip(endpoints.pos.get(), endpoints.rot.get());
    Layout l = editor_layout::build_layout(
        *ctx_.graph, ctx_.overrides ? *ctx_.overrides : editor_layout::PosOverrides{});

    const Card* hc = card_under(l, tip);
    std::string hit_card = hc ? hc->node_id : std::string{};

    // Nearest edge by midpoint of its endpoint handles.
    int hit_edge = -1;
    float best = 0.04f;
    for (int ei = 0; ei < int(ctx_.graph->edges.size()); ++ei) {
        Eigen::Vector3f f, t;
        if (!editor_layout::edge_endpoints(l, ctx_.graph->edges[std::size_t(ei)], f, t)) continue;
        float d = (tip - (f + t) * 0.5f).norm();
        if (d < best) {
            best = d;
            hit_edge = ei;
        }
    }

    if (hit_card == dwell_card_ && hit_edge == dwell_edge_ && (!hit_card.empty() || hit_edge >= 0))
        dwell_s_ += dt;
    else {
        dwell_s_ = 0.f;
        dwell_card_ = hit_card;
        dwell_edge_ = hit_edge;
    }

    if (dwell_s_ >= 1.0f) {
        dwell_s_ = 0.f;
        if (!dwell_card_.empty()) {
            ctx_.edits->push("{\"op\":\"remove_node\",\"id\":\"" + dwell_card_ + "\"}");
            dwell_card_.clear();
        } else if (dwell_edge_ >= 0) {
            const Edge& e = ctx_.graph->edges[std::size_t(dwell_edge_)];
            ctx_.edits->push(
                "{\"op\":\"remove_edge\",\"from\":\"" + e.from_node + "." + e.from_port +
                "\",\"to\":\"" + e.to_node + "." + e.to_port + "\"}");
            dwell_edge_ = -1;
        }
    }
}
