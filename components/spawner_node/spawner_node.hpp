// Copyright 2026 Travis West
#pragma once
#include "component_registry.hpp"
#include "editor_layout.hpp"
#include "event_queue.hpp"
#include "signal_graph.hpp"
#include "sygaldry_endpoints.hpp"
#include <string>
#include <string_view>

// Adds a node of `type` to the enclosing graph on each rising edge of
// `trigger`. Wire a ui_button into it and the graph grows its own editor.
// Context arrives through the generic ABI v9 seam before tick; a structured
// add_node op flows out through the queue mapping (apply_edit_op picks the
// first free "<type>_N" id).
struct SpawnerNode {
    static consteval std::string_view name() { return "spawner"; }
    static consteval std::string_view source_header() { return "components/spawner_node/spawner_node.hpp"; }
    static constexpr int lift_kind() { return EYEBALLS_LIFT_RESOURCE_HOLDER; }

    struct endpoints {
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> trigger;
        event_in spawn;  // event-rate command (poke_button etc.)
        normalled_in<std::string> type;
    } endpoints;

    void operator()(double) {
        bool on = endpoints.trigger.get() > 0.5f;
        bool fire = (on && !prev_on_) || endpoints.spawn.triggered;
        endpoints.spawn.triggered = false;   // events are deliveries
        prev_on_ = on;
        std::string type = endpoints.type.get();
        if (!fire || !registry_ || !edits_ || type.empty()) return;
        if (!registry_->find(type)) return;
        edits_->push(
            "{\"op\":\"add_node\",\"type\":\"" + editor_layout::json_escape(type) + "\"}");
    }

    void set_context(const Graph*, const ComponentRegistry* r,
                     EventQueue<std::string>* edits) {
        registry_ = r; edits_ = edits;
    }
    void set_host_context(const char* kind, void* ctx) {
        if (std::string_view{kind} != editor_layout::kEditorContextKind) return;
        const auto& g = *static_cast<const editor_layout::GestureContext*>(ctx);
        set_context(g.graph, g.registry, g.edits);
    }

private:
    bool prev_on_ = false;
    const ComponentRegistry* registry_ = nullptr;
    EventQueue<std::string>* edits_    = nullptr;
};
