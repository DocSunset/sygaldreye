// Copyright 2026 Travis West
#include "wire_drag.hpp"

#include "port_types.hpp"

using editor_layout::Card;
using editor_layout::Handle;
using editor_layout::Layout;

namespace {
// Nearest output handle to the tip within `radius` (rows are 0.018 m apart, so
// first-match grabbed neighbours instead of the aim — pick the closest).
const Handle* nearest_output(const Layout& l, const Eigen::Vector3f& tip, float radius) {
    const Handle* best = nullptr;
    for (const auto& c : l.cards)
        for (const auto& h : c.outputs) {
            float d = (tip - h.world_pos).norm();
            if (d < radius) {
                radius = d;
                best = &h;
            }
        }
    return best;
}

const Handle* nearest_compatible_input(
    const Layout& l, const Eigen::Vector3f& tip, const std::string& from_kind, float radius) {
    const Handle* best = nullptr;
    for (const auto& c : l.cards)
        for (const auto& h : c.inputs) {
            float d = (tip - h.world_pos).norm();
            if (d < radius && port_types::connection_legal(from_kind, h.port_kind)) {
                radius = d;
                best = &h;
            }
        }
    return best;
}

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

void WireDragNode::operator()(double) {
    if (!ctx_.graph) return;
    Eigen::Vector3f tip = editor_layout::controller_tip(endpoints.pos.get(), endpoints.rot.get());
    bool grip = endpoints.grip.get() > 0.5f;
    bool grip_down = grip && !prev_grip_;
    bool grip_up = !grip && prev_grip_;
    prev_grip_ = grip;

    Layout l = editor_layout::build_layout(
        *ctx_.graph, ctx_.overrides ? *ctx_.overrides : editor_layout::PosOverrides{});

    if (state_ == State::MovingCard) {
        if (!grip) {
            state_ = State::Idle;
            move_id_.clear();
        } else if (ctx_.overrides && !move_id_.empty()) {
            (*ctx_.overrides)[move_id_] = tip + move_offset_;
        }
        return;
    }

    if (state_ == State::Idle && grip_down) {
        if (const Handle* h = nearest_output(l, tip, 0.02f)) {
            state_ = State::Dragging;
            from_node_ = h->node_id;
            from_port_ = h->port_name;
            from_kind_ = h->port_kind;
            return;
        }
        if (const Card* c = card_under(l, tip)) {
            state_ = State::MovingCard;
            move_id_ = c->node_id;
            move_offset_ = c->position - tip;
        }
        return;
    }

    if (state_ == State::Dragging && grip_up) {
        state_ = State::Idle;
        if (const Handle* h = nearest_compatible_input(l, tip, from_kind_, 0.03f)) {
            if (ctx_.edits) {
                std::string op = "{\"op\":\"add_edge\",\"from\":\"" + from_node_ + "." +
                                 from_port_ + "\",\"to\":\"" + h->node_id + "." + h->port_name +
                                 "\"}";
                ctx_.edits->push(std::move(op));
            }
        }
    }
}
