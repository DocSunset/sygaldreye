// Copyright 2025 Travis West
#include "vr_editor.hpp"
#include "ray_selector.hpp"
#include <Eigen/Geometry>
#include <cstdio>
#include <ctime>
#include <tuple>

static constexpr float kCardSpacing = 0.45f;
static constexpr float kCardW       = 0.4f;
static constexpr float kBaseCardH   = 0.08f;
static constexpr float kPortRowH    = 0.018f;
static constexpr float kPaletteRowH = 0.06f;

// Declared in vr_editor_handles.cpp
std::tuple<std::vector<PortHandle>,
           std::vector<PortHandle>,
           std::vector<SliderWidget>>
build_port_handles_for_card(const VrPanel&, const std::string&, const PortSchema&);

void VrEditor::init(const ComponentRegistry& registry, const Graph* graph) {
    shader_.create();
    palette_types_ = registry.type_names();
    float palette_h = kPaletteRowH * static_cast<float>(palette_types_.size());
    palette_panel_.position = {-0.3f, 0.0f, -0.5f};
    palette_panel_.normal   = { 0.0f, 0.0f,  1.0f};
    palette_panel_.width    = 0.35f;
    palette_panel_.height   = std::max(palette_h, 0.1f);
    palette_panel_.color    = {0.05f, 0.08f, 0.12f, 0.90f};
    on_graph_changed(graph);
}

void VrEditor::on_graph_changed(const Graph* graph) {
    node_cards_.clear();
    card_ids_.clear();
    input_handles_.clear();
    output_handles_.clear();
    sliders_.clear();
    current_edges_.clear();

    if (!graph) return;

    current_edges_ = graph->edges;

    for (int i = 0; i < static_cast<int>(graph->nodes.size()); ++i) {
        const auto& n = graph->nodes[static_cast<size_t>(i)];
        PortSchema schema = parse_port_schema(n.desc ? n.desc->port_schema : nullptr);

        int wirable_inputs = 0;
        for (const auto& p : schema.inputs)
            if (p.is_wirable()) ++wirable_inputs;

        float card_h = kBaseCardH + kPortRowH * static_cast<float>(wirable_inputs);

        VrPanel card;
        card.position = {static_cast<float>(i) * kCardSpacing, 0.0f, -0.5f};
        card.normal   = {0.0f, 0.0f, 1.0f};
        card.width    = kCardW;
        card.height   = card_h;
        card.color    = {0.08f, 0.12f, 0.18f, 0.88f};
        node_cards_.push_back(card);
        card_ids_.push_back(n.id);

        auto [in_h, out_h, sl] = build_port_handles_for_card(card, n.id, schema);
        input_handles_.push_back(std::move(in_h));
        output_handles_.push_back(std::move(out_h));
        sliders_.push_back(std::move(sl));
    }
}

std::optional<VrEditor::GraphEdit> VrEditor::update(
        const XrPosef* /*left_pose*/,
        const XrPosef* right_pose,
        bool /*trigger_left*/,
        bool trigger_right,
        bool grip_right,
        const Eigen::Vector2f& thumbstick_left,
        float delta_time_s,
        const Graph* current_graph,
        const ComponentRegistry& registry) {

    if (right_pose) {
        const auto& q = right_pose->orientation;
        Eigen::Quaternionf eq(q.w, q.x, q.y, q.z);
        Eigen::Vector3f fwd = eq * Eigen::Vector3f{0, 0, -1};
        controller_tip_ = {right_pose->position.x + fwd.x() * 0.05f,
                           right_pose->position.y + fwd.y() * 0.05f,
                           right_pose->position.z + fwd.z() * 0.05f};
    }

    bool fire = trigger_right && !prev_trigger_right_;
    prev_trigger_right_ = trigger_right;

    if (auto e = update_drag(grip_right, current_graph))         return e;
    if (auto e = update_sliders(right_pose, trigger_right, current_graph)) return e;
    if (auto e = update_dwell(right_pose, delta_time_s, current_graph))    return e;
    if (auto e = update_undo(thumbstick_left))                   return e;

    if (!fire || !right_pose || palette_types_.empty()) return std::nullopt;
    auto hit = RaySelector::test(*right_pose, {palette_panel_});
    if (!hit) return std::nullopt;

    float inv_y = 1.0f - hit->hit.uv.y();
    int idx = static_cast<int>(inv_y * static_cast<float>(palette_types_.size()));
    if (idx < 0 || idx >= static_cast<int>(palette_types_.size())) return std::nullopt;
    const std::string& type_name = palette_types_[static_cast<size_t>(idx)];

    const auto* desc = registry.find(type_name);
    if (!desc) return std::nullopt;

    char id_buf[32];
    std::snprintf(id_buf, sizeof(id_buf), "%s_%ld", type_name.c_str(),
                  static_cast<long>(std::time(nullptr)));

    std::string json = current_graph ? serialize_graph(*current_graph)
                                     : std::string{"{\"nodes\":[],\"edges\":[]}"};
    undo_json_ = json;

    auto pos = json.rfind(']');
    if (pos == std::string::npos) return std::nullopt;
    std::string entry = (pos > 0 && json[pos-1] != '[') ? "," : "";
    entry += "{\"id\":\""; entry += id_buf; entry += "\",\"type\":\"";
    entry += type_name; entry += "\"}";
    json.insert(pos, entry);

    return GraphEdit{std::move(json)};
}
