// Copyright 2026 Travis West
#include "editor_layout.hpp"

#include <cstdint>
#include <cstdlib>
#include <string_view>

#include "port_schema_reader.hpp"

namespace editor_layout {

float id_key(const std::string& id) {
    std::uint32_t h = 2166136261u;  // FNV-1a (same as graph_source)
    for (unsigned char c : id) {
        h ^= c;
        h *= 16777619u;
    }
    return float(h & 0xffffffu);
}

Eigen::Vector3f default_card_pos(int i) {
    return {
        float(i % 8) * kCardSpacing - 1.6f + float(i / 32) * 4.0f,
        1.85f - float((i / 8) % 4) * 0.5f,
        -0.5f};
}

namespace {
// Seed slider displays from the node's live serialized params — widgets
// default to 0 otherwise (vr_editor.cpp on_graph_changed).
void seed_slider_values(const NodeInstance& n, std::vector<Slider>& sliders) {
    if (!n.desc || !n.desc->serialize || !n.data) return;
    const char* s = n.desc->serialize(n.data);
    if (!s) return;
    std::string_view js{s};
    for (auto& w : sliders) {
        std::string needle = "\"" + w.port_name + "\":";
        if (auto p = js.find(needle); p != std::string_view::npos)
            w.value = std::strtof(js.data() + p + needle.size(), nullptr);
    }
    if (n.desc->free_str) n.desc->free_str(s);
}
}  // namespace

Layout build_layout(
    const Graph& g, const std::unordered_map<std::string, Eigen::Vector3f>& overrides) {
    Layout l;
    for (int i = 0; i < int(g.nodes.size()); ++i) {
        const NodeInstance& n = g.nodes[std::size_t(i)];
        PortSchema schema = parse_port_schema(n.desc ? n.desc->port_schema : nullptr);

        int wirable_inputs = 0;
        for (const auto& p : schema.inputs)
            if (p.is_wirable()) ++wirable_inputs;

        Card card;
        card.node_id = n.id;
        auto ov = overrides.find(n.id);
        card.position = (ov != overrides.end()) ? ov->second : default_card_pos(i);
        card.width = kCardW;
        card.height = kBaseCardH + kPortRowH * float(wirable_inputs);

        const float top = card.position.y() + card.height * 0.5f - kHandleSize;
        const float left_x = card.position.x() - card.width * 0.5f - kHandleOffX;
        const float right_x = card.position.x() + card.width * 0.5f + kHandleOffX;
        const float z = card.position.z() + 0.003f;

        int row = 0;
        for (const auto& p : schema.inputs) {
            if (!p.is_wirable()) continue;
            float y = top - float(row) * kPortRowH;
            card.inputs.push_back({n.id, p.name, p.kind, false, {left_x, y, z}});
            if (p.kind == "scalar")
                card.sliders.push_back(
                    {n.id,
                     p.name,
                     {card.position.x(), y, z},
                     card.width * 0.6f,
                     p.min,
                     p.max,
                     0.f});
            ++row;
        }
        row = 0;
        for (const auto& p : schema.outputs) {
            if (!p.is_wirable()) continue;
            float y = top - float(row) * kPortRowH;
            card.outputs.push_back({n.id, p.name, p.kind, true, {right_x, y, z}});
            ++row;
        }
        seed_slider_values(n, card.sliders);
        l.cards.push_back(std::move(card));
    }
    return l;
}

bool edge_endpoints(const Layout& l, const Edge& e, Eigen::Vector3f& from, Eigen::Vector3f& to) {
    bool hf = false, ht = false;
    for (const auto& c : l.cards) {
        if (c.node_id == e.from_node)
            for (const auto& h : c.outputs)
                if (h.port_name == e.from_port) {
                    from = h.world_pos;
                    hf = true;
                }
        if (c.node_id == e.to_node)
            for (const auto& h : c.inputs)
                if (h.port_name == e.to_port) {
                    to = h.world_pos;
                    ht = true;
                }
    }
    return hf && ht;
}

}  // namespace editor_layout
