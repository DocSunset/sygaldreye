// Copyright 2025 Travis West
#include "vr_editor.hpp"
#include "ray_selector.hpp"
#include <Eigen/Geometry>
#include <cstdio>
#include <ctime>

static constexpr float kCardSpacing = 0.45f;
static constexpr float kCardW = 0.4f;
static constexpr float kCardH = 0.12f;
static constexpr float kPaletteRowH = 0.06f;

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
    if (!graph) return;
    for (int i = 0; i < static_cast<int>(graph->nodes.size()); ++i) {
        const auto& n = graph->nodes[static_cast<size_t>(i)];
        VrPanel card;
        card.position = {static_cast<float>(i) * kCardSpacing, 0.0f, -0.5f};
        card.normal   = {0.0f, 0.0f, 1.0f};
        card.width    = kCardW;
        card.height   = kCardH;
        card.color    = {0.08f, 0.12f, 0.18f, 0.88f};
        node_cards_.push_back(card);
        card_ids_.push_back(n.id);
    }
}

std::optional<VrEditor::GraphEdit> VrEditor::update(
        const XrPosef* /*left_pose*/,
        const XrPosef* right_pose,
        bool /*trigger_left*/,
        bool trigger_right,
        const Graph* current_graph,
        const ComponentRegistry& registry) {

    bool fire = trigger_right && !prev_trigger_right_;
    prev_trigger_right_ = trigger_right;

    if (!fire || !right_pose || palette_types_.empty()) return std::nullopt;

    auto hit = RaySelector::test(*right_pose, {palette_panel_});
    if (!hit) return std::nullopt;

    // Map UV.y to type index (0 = top row)
    float inv_y = 1.0f - hit->hit.uv.y();
    int idx = static_cast<int>(inv_y * static_cast<float>(palette_types_.size()));
    if (idx < 0 || idx >= static_cast<int>(palette_types_.size())) return std::nullopt;
    const std::string& type_name = palette_types_[static_cast<size_t>(idx)];

    const auto* desc = registry.find(type_name);
    if (!desc) return std::nullopt;

    // Build new graph JSON appending one node
    char id_buf[32];
    std::snprintf(id_buf, sizeof(id_buf), "%s_%ld", type_name.c_str(),
                  static_cast<long>(std::time(nullptr)));

    std::string json = current_graph ? serialize_graph(*current_graph)
                                     : std::string{"{\"nodes\":[],\"edges\":[]}"};
    // Inject new node before closing bracket
    auto pos = json.rfind(']');
    if (pos == std::string::npos) return std::nullopt;
    std::string entry = (pos > 0 && json[pos-1] != '[') ? "," : "";
    entry += "{\"id\":\""; entry += id_buf; entry += "\",\"type\":\"";
    entry += type_name; entry += "\"}";
    json.insert(pos, entry);

    return GraphEdit{std::move(json)};
}

void VrEditor::draw(const Eigen::Matrix4f& vp, const TextMesh& text) const {
    palette_panel_.draw(vp, shader_);

    // Draw type labels
    float n = static_cast<float>(palette_types_.size());
    for (int i = 0; i < static_cast<int>(palette_types_.size()); ++i) {
        float v = (static_cast<float>(i) + 0.5f) / n;
        float y = palette_panel_.position.y() +
                  palette_panel_.height * (0.5f - v);
        Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
        m(0,0) = m(1,1) = 0.02f;
        m(0,3) = palette_panel_.position.x() - palette_panel_.width * 0.45f;
        m(1,3) = y;
        m(2,3) = palette_panel_.position.z() + 0.002f;
        text.draw(palette_types_[static_cast<size_t>(i)], vp * m);
    }

    // Draw node cards
    for (int i = 0; i < static_cast<int>(node_cards_.size()); ++i) {
        node_cards_[static_cast<size_t>(i)].draw(vp, shader_);
        Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
        m(0,0) = m(1,1) = 0.018f;
        const auto& c = node_cards_[static_cast<size_t>(i)];
        m(0,3) = c.position.x() - c.width * 0.45f;
        m(1,3) = c.position.y();
        m(2,3) = c.position.z() + 0.002f;
        text.draw(card_ids_[static_cast<size_t>(i)], vp * m);
    }
}
