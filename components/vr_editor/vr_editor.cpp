// Copyright 2025 Travis West
#include "vr_editor.hpp"
#include <Eigen/Geometry>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string_view>
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
    std::sort(palette_types_.begin(), palette_types_.end());
    palette_panel_.position = {-0.55f, 1.2f, -0.5f};
    palette_panel_.normal   = { 0.0f, 0.0f,  1.0f};
    palette_panel_.width    = 0.35f;
    palette_panel_.height   = kPaletteRowH * static_cast<float>(kPaletteRows + 1);
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
        // Reachable band: 8 per row, 4 rows, then a fresh block further
        // right — the 4-wide grid sank below the floor by row 8.
        card.position = {static_cast<float>(i % 8) * kCardSpacing - 1.6f +
                             static_cast<float>(i / 32) * 4.0f,
                         1.85f - static_cast<float>((i / 8) % 4) * 0.5f,
                         -0.5f};
        card.normal   = {0.0f, 0.0f, 1.0f};
        card.width    = kCardW;
        card.height   = card_h;
        card.color    = {0.08f, 0.12f, 0.18f, 0.88f};
        node_cards_.push_back(card);
        card_ids_.push_back(n.id);

        auto ov = card_pos_ovr_.find(n.id);
        if (ov != card_pos_ovr_.end()) card.position = ov->second;
        node_cards_.back() = card;

        auto [in_h, out_h, sl] = build_port_handles_for_card(card, n.id, schema);
        // Seed slider displays from the node's live params — widgets default
        // to 0 otherwise, so every rebuild snapped displays back.
        if (n.desc && n.desc->serialize && n.data) {
            if (const char* s = n.desc->serialize(n.data)) {
                std::string_view js{s};
                for (auto& w : sl) {
                    std::string needle = "\"" + w.port_name + "\":";
                    auto p = js.find(needle);
                    if (p != std::string_view::npos)
                        w.value = std::strtof(js.data() + p + needle.size(), nullptr);
                }
                if (n.desc->free_str) n.desc->free_str(s);
            }
        }
        input_handles_.push_back(std::move(in_h));
        output_handles_.push_back(std::move(out_h));
        sliders_.push_back(std::move(sl));
    }
}

// Re-derive one card's handle/slider layout after it moves.
void VrEditor::rebuild_card(std::size_t idx, const ComponentRegistry*) {
    if (idx >= node_cards_.size()) return;
    // Preserve live slider values across the rebuild.
    std::unordered_map<std::string, float> vals;
    for (const auto& sl : sliders_[idx]) vals[sl.port_name] = sl.value;
    PortSchema schema;  // rebuild from the same ports the card was built with
    for (const auto& h : input_handles_[idx])
        schema.inputs.push_back({h.port_name, h.port_kind, 0.f, 1.f});
    for (const auto& sl : sliders_[idx]) {
        for (auto& pi : schema.inputs)
            if (pi.name == sl.port_name) { pi.min = sl.min_val; pi.max = sl.max_val; }
    }
    for (const auto& h : output_handles_[idx])
        schema.outputs.push_back({h.port_name, h.port_kind, 0.f, 1.f});
    auto [in_h, out_h, sl] = build_port_handles_for_card(
        node_cards_[idx], card_ids_[idx], schema);
    for (auto& w : sl) { auto it = vals.find(w.port_name); if (it != vals.end()) w.value = it->second; }
    input_handles_[idx]  = std::move(in_h);
    output_handles_[idx] = std::move(out_h);
    sliders_[idx]        = std::move(sl);
}

std::optional<VrEditor::GraphEdit> VrEditor::update(
        const XrPosef* left_pose,
        const XrPosef* right_pose,
        bool trigger_left,
        bool trigger_right,
        bool grip_right,
        const Eigen::Vector2f& thumbstick_left,
        float delta_time_s,
        const Graph* current_graph,
        const ComponentRegistry& registry) {

    show_left_ = left_pose != nullptr;
    show_right_ = right_pose != nullptr;
    trig_l_ = trigger_left; trig_r_ = trigger_right; grip_r_ = grip_right;
    if (left_pose)
        left_tip_ = Eigen::Vector3f{left_pose->position.x, left_pose->position.y,
                                    left_pose->position.z};
    if (right_pose) {
        const auto& q = right_pose->orientation;
        Eigen::Quaternionf eq(q.w, q.x, q.y, q.z);
        Eigen::Vector3f fwd = eq * Eigen::Vector3f{0, 0, -1};
        controller_tip_ = {right_pose->position.x + fwd.x() * 0.05f,
                           right_pose->position.y + fwd.y() * 0.05f,
                           right_pose->position.z + fwd.z() * 0.05f};
        right_tip_  = controller_tip_;
        ray_origin_ = controller_tip_;
        ray_dir_    = fwd;
    }

    // Hover: nearest port handle to the tip, or an actively-near slider.
    hover_label_.clear();
    float best_hover = 0.05f;
    for (std::size_t ci = 0; ci < node_cards_.size(); ++ci) {
        for (const auto& h : input_handles_[ci]) {
            float d = (controller_tip_ - h.world_pos).norm();
            if (d < best_hover) { best_hover = d; hover_label_ = h.port_name + " ◂"; hover_pos_ = h.world_pos; }
        }
        for (const auto& h : output_handles_[ci]) {
            float d = (controller_tip_ - h.world_pos).norm();
            if (d < best_hover) { best_hover = d; hover_label_ = "▸ " + h.port_name; hover_pos_ = h.world_pos; }
        }
        for (const auto& sl : sliders_[ci]) {
            Eigen::Vector3f delta = controller_tip_ - sl.world_pos;
            if (std::abs(delta.x()) < sl.width * 0.5f &&
                std::abs(delta.y()) < 0.03f && std::abs(delta.z()) < 0.06f) {
                char buf[96];
                std::snprintf(buf, sizeof(buf), "%s = %.3g", sl.port_name.c_str(), double(sl.value));
                hover_label_ = buf;
                hover_pos_   = sl.world_pos;
            }
        }
    }

    // Card move: grip on a card body (not near an output handle).
    if (drag_state_ == DragState::MovingCard) {
        if (!grip_right) {
            drag_state_  = DragState::Idle;
            moving_card_ = -1;
        } else if (moving_card_ >= 0) {
            auto idx = std::size_t(moving_card_);
            node_cards_[idx].position = controller_tip_ + move_grab_offset_;
            card_pos_ovr_[card_ids_[idx]] = node_cards_[idx].position;
            rebuild_card(idx);
            return std::nullopt;  // suppress other interactions while moving
        }
    }

    bool fire = trigger_right && !prev_trigger_right_;
    prev_trigger_right_ = trigger_right;

    if (auto e = update_drag(grip_right, current_graph))         return e;
    if (auto e = update_sliders(right_pose, trigger_right, current_graph)) return e;
    if (auto e = update_dwell(right_pose, grip_right, delta_time_s, current_graph)) return e;
    if (auto e = update_undo(thumbstick_left))                   return e;

    if (!fire || !right_pose || palette_types_.empty()) return std::nullopt;
    // Poke, not ray: trigger with the tip in the panel. Row 0 flips pages.
    Eigen::Vector3f d = controller_tip_ - palette_panel_.position;
    if (std::abs(d.x()) > palette_panel_.width * 0.5f ||
        std::abs(d.y()) > palette_panel_.height * 0.5f ||
        std::abs(d.z()) > 0.08f) return std::nullopt;
    int row = static_cast<int>((0.5f - d.y() / palette_panel_.height) *
                               static_cast<float>(kPaletteRows + 1));
    if (row <= 0) {
        palette_page_ = (palette_page_ + 1) % palette_pages();
        return std::nullopt;
    }
    int idx = palette_page_ * kPaletteRows + row - 1;
    if (idx >= static_cast<int>(palette_types_.size())) return std::nullopt;
    const std::string& type_name = palette_types_[static_cast<size_t>(idx)];

    const auto* desc = registry.find(type_name);
    if (!desc) return std::nullopt;

    char id_buf[32];
    std::snprintf(id_buf, sizeof(id_buf), "%s_%ld", type_name.c_str(),
                  static_cast<long>(std::time(nullptr)));

    std::string json = current_graph ? serialize_graph(*current_graph)
                                     : std::string{"{\"nodes\":[],\"edges\":[]}"};
    undo_json_ = json;

    // Insert into the nodes array — the last ']' in the document closes edges.
    auto epos = json.find("\"edges\"");
    auto pos  = (epos == std::string::npos) ? json.rfind(']')
                                            : json.rfind(']', epos);
    if (pos == std::string::npos) return std::nullopt;
    std::string entry = (pos > 0 && json[pos-1] != '[') ? "," : "";
    entry += "{\"id\":\""; entry += id_buf; entry += "\",\"type\":\"";
    entry += type_name; entry += "\"}";
    json.insert(pos, entry);

    return GraphEdit{std::move(json)};
}
