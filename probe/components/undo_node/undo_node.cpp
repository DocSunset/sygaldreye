// Copyright 2026 Travis West
#include "undo_node.hpp"

namespace {
// A STRUCTURAL signature: node ids/types + edges, ignoring param drift (camera
// pose, slider values wobble every frame — diffing the full serialization
// overwrote the undo target each tick). The undo target is the graph BEFORE the
// last structural edit (a node/edge add or remove), like the monolith's
// undo_json_ captured before each edit.
std::string structure_sig(const Graph& g) {
    std::string s;
    for (const auto& n : g.nodes) {
        s += n.id;
        s += ':';
        s += n.desc ? n.desc->type_name : "?";
        s += ';';
    }
    s += '|';
    for (const auto& e : g.edges) {
        s += e.from_node;
        s += '.';
        s += e.from_port;
        s += '>';
        s += e.to_node;
        s += '.';
        s += e.to_port;
        s += ';';
    }
    return s;
}
}  // namespace

void UndoNode::operator()(double) {
    if (!ctx_.graph) return;

    // Snapshot the graph only when its STRUCTURE changes; the prior snapshot is
    // the undo target. Param-only frames don't disturb it.
    std::string sig = structure_sig(*ctx_.graph);
    std::string now = serialize_graph(*ctx_.graph);
    if (current_sig_.empty()) {
        current_sig_ = sig;
        current_ = now;
    } else if (sig != current_sig_) {
        previous_ = current_;
        current_sig_ = sig;
        current_ = now;
    } else {
        current_ = now;  // keep the latest params for the current structure
    }

    bool gesture = endpoints.thumb_x.get() < -0.7f;
    bool rising = gesture && !prev_gesture_;
    prev_gesture_ = gesture;

    if (rising && !previous_.empty() && ctx_.edits) {
        ctx_.edits->push(previous_);  // whole-graph edit passes through verbatim
        previous_.clear();
        current_sig_.clear();  // re-baseline on the restored graph next tick
    }
}
