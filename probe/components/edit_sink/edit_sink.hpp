// Copyright 2026 Travis West
#pragma once
#include <string>
#include <string_view>

#include "event_queue.hpp"
#include "sygaldry_endpoints.hpp"

// The editor's edit drain (vr_editor_decomposition.md): consumes structured
// edit-op commands (text) and pushes them onto the SAME edit queue
// PeerCore::collect_edits drains. Replaces EditorNode's whole-graph-JSON
// splicing — gesture nodes emit small structured ops here instead.
//
// One op per command, applied to the live graph in collect_edits via
// apply_edit_op (signal_graph_edit). A resource-holder (it owns the queue
// seam, injected by the shell); unliftable.
//
// Op vocabulary (text command on `command`, deduped on rising edge):
//   {"op":"add_node","type":"<t>"[,"id":"<id>"]}
//   {"op":"remove_node","id":"<id>"}
//   {"op":"add_edge","from":"<n.p>","to":"<n.p>"}
//   {"op":"remove_edge","from":"<n.p>","to":"<n.p>"}
//   {"op":"set_param","id":"<id>","port":"<p>","value":<f>}
// A command that is a whole graph object (starts with a "nodes" key) is
// passed through verbatim — keeps the old editor's full-graph edits working.
struct EditSinkNode {
    static consteval std::string_view name() { return "edit_sink"; }
    static consteval std::string_view source_header() {
        return "components/edit_sink/edit_sink.hpp";
    }
    static constexpr int lift_kind() { return lift::resource_holder; }

    struct endpoints {
        normalled_in<std::string> command;  // one op-command JSON
    } endpoints;

    void operator()(double) {
        std::string cmd = endpoints.command.get();
        if (cmd.empty() || cmd == last_ || !edits_) return;
        last_ = cmd;
        edits_->push(std::move(cmd));
    }

    void set_context(EventQueue<std::string>* edits) { edits_ = edits; }

private:
    std::string last_;
    EventQueue<std::string>* edits_ = nullptr;
};
