// Copyright 2026 Travis West
#pragma once
#include "signal_graph.hpp"
#include "sygaldry_endpoints.hpp"
#include "event_queue.hpp"
#include <ctime>
#include <string>
#include <string_view>

// Adds a node of `type` to the enclosing graph on each rising edge of
// `trigger`. Wire a ui_button into it and the graph grows its own editor.
// Same platform seams as the editor node: set_context() before tick;
// edits flow out through the queue mapping.
struct SpawnerNode {
    static consteval std::string_view name() { return "spawner"; }
    static consteval std::string_view source_header() { return "components/spawner_node/spawner_node.hpp"; }

    struct inputs {
        slider<"trigger", "", float, fp(0.f), fp(1.f), fp(0.f)> trigger;
        bang<"spawn"> spawn;  // event-rate command (poke_button etc.)
        ::text<"type"> type;
    } inputs;
    struct outputs {} outputs;

    void operator()(double) {
        bool on = inputs.trigger.value > 0.5f;
        bool fire = (on && !prev_on_) || inputs.spawn.triggered;
        prev_on_ = on;
        if (!fire || !graph_ || !registry_ || inputs.type.value.empty()) return;
        if (!registry_->find(inputs.type.value)) return;

        std::string json = serialize_graph(*graph_);
        auto epos = json.find("\"edges\"");
        auto pos  = (epos == std::string::npos) ? json.rfind(']')
                                                : json.rfind(']', epos);
        if (pos == std::string::npos) return;
        char id[64];
        std::snprintf(id, sizeof(id), "%s_%ld", inputs.type.value.c_str(),
                      static_cast<long>(std::time(nullptr)));
        std::string entry = (pos > 0 && json[pos-1] != '[') ? "," : "";
        entry += "{\"id\":\""; entry += id; entry += "\",\"type\":\"";
        entry += inputs.type.value; entry += "\"}";
        json.insert(pos, entry);
        if (edits_) edits_->push(std::move(json));
    }

    void set_context(const Graph* g, const ComponentRegistry* r,
                     EventQueue<std::string>* edits) {
        graph_ = g; registry_ = r; edits_ = edits;
    }

private:
    bool prev_on_ = false;
    const Graph*             graph_    = nullptr;
    const ComponentRegistry* registry_ = nullptr;
    EventQueue<std::string>* edits_    = nullptr;
};
