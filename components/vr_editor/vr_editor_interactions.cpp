// Copyright 2025 Travis West
// VrEditor interaction sub-update helpers: drag, sliders, dwell, undo
#include "vr_editor.hpp"
#include "port_types.hpp"
#include "ray_selector.hpp"
#include <Eigen/Geometry>
#include <cstring>

// ── helpers ─────────────────────────────────────────────────────────────────

// Remove all edges referencing node_id, return new graph JSON
static std::string remove_node(const std::string& cur_json, const std::string& node_id) {
    // Build new graph by re-parsing and dropping the node + its edges.
    // Minimal approach: find and remove {"id":"<id>",...} and edges referencing it.
    // We use serialize_graph after manipulating the JSON string directly.
    // (A proper parse would require signal_graph; we do string surgery instead.)

    // Remove node entry: find "id":"<node_id>" block
    std::string out = cur_json;
    std::string id_needle = "\"id\":\"" + node_id + "\"";
    auto id_pos = out.find(id_needle);
    if (id_pos != std::string::npos) {
        // Find start of enclosing object (search backwards for '{')
        auto obj_start = out.rfind('{', id_pos);
        if (obj_start != std::string::npos) {
            // Find end
            int depth = 0;
            size_t obj_end = obj_start;
            for (size_t i = obj_start; i < out.size(); ++i) {
                if (out[i] == '{') ++depth;
                else if (out[i] == '}') { if (--depth == 0) { obj_end = i; break; } }
            }
            // Remove including possible leading comma
            size_t remove_start = obj_start;
            if (remove_start > 0 && out[remove_start-1] == ',')
                --remove_start;
            else if (obj_end + 1 < out.size() && out[obj_end+1] == ',')
                ++obj_end;
            out.erase(remove_start, obj_end - remove_start + 1);
        }
    }

    // Remove edges referencing node_id
    std::string from_ref = "\"" + node_id + ".";
    std::string to_ref   = "\"" + node_id + ".";
    // Iterate and remove any edge object containing from_node or to_node == node_id
    std::string edge_needle_from = "\"from\":\"" + node_id + ".";
    std::string edge_needle_to   = "\"to\":\""   + node_id + ".";

    auto remove_edge_containing = [&](const std::string& needle) {
        size_t p = out.find(needle);
        while (p != std::string::npos) {
            auto obj_start = out.rfind('{', p);
            if (obj_start == std::string::npos) break;
            int depth = 0;
            size_t obj_end = obj_start;
            for (size_t i = obj_start; i < out.size(); ++i) {
                if (out[i] == '{') ++depth;
                else if (out[i] == '}') { if (--depth == 0) { obj_end = i; break; } }
            }
            size_t rs = obj_start;
            if (rs > 0 && out[rs-1] == ',') --rs;
            else if (obj_end+1 < out.size() && out[obj_end+1] == ',') ++obj_end;
            out.erase(rs, obj_end - rs + 1);
            p = out.find(needle);
        }
    };
    remove_edge_containing(edge_needle_from);
    remove_edge_containing(edge_needle_to);
    return out;
}

static std::string remove_edge_at(const std::string& cur_json, size_t edge_idx,
                                   const std::vector<Edge>& edges) {
    if (edge_idx >= edges.size()) return cur_json;
    const auto& e = edges[edge_idx];
    std::string from_val = e.from_node + "." + e.from_port;
    std::string to_val   = e.to_node   + "." + e.to_port;
    std::string needle = "\"from\":\"" + from_val + "\",\"to\":\"" + to_val;
    std::string out = cur_json;
    auto p = out.find(needle);
    if (p == std::string::npos) return out;
    auto obj_start = out.rfind('{', p);
    if (obj_start == std::string::npos) return out;
    int depth = 0;
    size_t obj_end = obj_start;
    for (size_t i = obj_start; i < out.size(); ++i) {
        if (out[i] == '{') ++depth;
        else if (out[i] == '}') { if (--depth == 0) { obj_end = i; break; } }
    }
    size_t rs = obj_start;
    if (rs > 0 && out[rs-1] == ',') --rs;
    else if (obj_end+1 < out.size() && out[obj_end+1] == ',') ++obj_end;
    out.erase(rs, obj_end - rs + 1);
    return out;
}

// ── drag-to-connect ──────────────────────────────────────────────────────────

std::optional<VrEditor::GraphEdit> VrEditor::update_drag(
        bool grip_right, const Graph* current_graph) {

    bool grip_edge_down = grip_right && !prev_grip_right_;
    bool grip_edge_up   = !grip_right && prev_grip_right_;
    prev_grip_right_ = grip_right;

    if (drag_state_ == DragState::Idle && grip_edge_down) {
        // Nearest output handle within radius — rows are 0.018 m apart, so
        // first-match within 0.02 m grabbed neighbours instead of the aim.
        const PortHandle* best = nullptr;
        float best_d = 0.02f;
        for (size_t ci = 0; ci < node_cards_.size(); ++ci)
            for (const auto& h : output_handles_[ci]) {
                float d = (controller_tip_ - h.world_pos).norm();
                if (d < best_d) { best_d = d; best = &h; }
            }
        if (best) {
            drag_state_       = DragState::Dragging;
            drag_from_node_   = best->node_id;
            drag_from_port_   = best->port_name;
            drag_from_kind_   = best->port_kind;
            drag_from_pos_    = best->world_pos;
            return std::nullopt;
        }
        // No handle under the tip: grab the card body to move it.
        for (std::size_t ci = 0; ci < node_cards_.size(); ++ci) {
            const auto& c = node_cards_[ci];
            Eigen::Vector3f d = controller_tip_ - c.position;
            if (std::abs(d.x()) < c.width * 0.5f &&
                std::abs(d.y()) < c.height * 0.5f &&
                std::abs(d.z()) < 0.08f) {
                drag_state_       = DragState::MovingCard;
                moving_card_      = int(ci);
                move_grab_offset_ = c.position - controller_tip_;
                return std::nullopt;
            }
        }
    }

    if (drag_state_ == DragState::Dragging && grip_edge_up) {
        drag_state_ = DragState::Idle;
        // Nearest kind-compatible input handle within radius.
        const PortHandle* best = nullptr;
        float best_d = 0.03f;
        for (size_t ci = 0; ci < node_cards_.size(); ++ci)
            for (const auto& h2 : input_handles_[ci]) {
                float d = (controller_tip_ - h2.world_pos).norm();
                if (d < best_d &&
                    port_types::connection_legal(drag_from_kind_, h2.port_kind)) {
                    best_d = d; best = &h2;
                }
            }
        if (best) {
            {
                const auto& h = *best;
                {
                    // Build new graph JSON with the new edge
                    if (!current_graph) return std::nullopt;
                    std::string json = serialize_graph(*current_graph);
                    undo_json_ = json;
                    // Insert new edge before closing bracket of edges array
                    std::string edge_entry =
                        "{\"from\":\"" + drag_from_node_ + "." + drag_from_port_ +
                        "\",\"to\":\""  + h.node_id      + "." + h.port_name + "\"}";
                    // Find last ']' after "edges":
                    auto edges_pos = json.find("\"edges\":");
                    if (edges_pos == std::string::npos) return std::nullopt;
                    auto arr_end = json.find(']', edges_pos);
                    if (arr_end == std::string::npos) return std::nullopt;
                    std::string sep = (arr_end > 0 && json[arr_end-1] != '[') ? "," : "";
                    json.insert(arr_end, sep + edge_entry);
                    return GraphEdit{std::move(json)};
                }
            }
        }
    }
    return std::nullopt;
}

// ── slider interaction ───────────────────────────────────────────────────────

std::optional<VrEditor::GraphEdit> VrEditor::update_sliders(
        const XrPosef* right_pose, bool trigger_right, const Graph* current_graph) {
    if (!right_pose || !current_graph) return std::nullopt;

    // One slider at a time: rows are 0.018 m apart, so a wide y-tolerance
    // swept neighbours during vertical drags. Pick the single nearest track
    // to the controller tip.
    SliderWidget* best = nullptr;
    float best_dy = 0.009f;  // half a port row
    for (size_t ci = 0; ci < sliders_.size(); ++ci) {
        for (auto& sl : sliders_[ci]) {
            sl.active = false;
            Eigen::Vector3f delta = controller_tip_ - sl.world_pos;
            if (std::abs(delta.x()) < sl.width * 0.5f &&
                std::abs(delta.z()) < 0.05f &&
                std::abs(delta.y()) < best_dy) {
                best_dy = std::abs(delta.y());
                best = &sl;
            }
        }
    }

    bool any_changed = false;
    if (best && trigger_right) {
        auto& sl = *best;
        sl.active = true;
        float dx = controller_tip_.x() - sl.world_pos.x();
        float norm = (dx + sl.width * 0.5f) / sl.width;
        norm = std::max(0.f, std::min(1.f, norm));
        sl.value = sl.min_val + norm * (sl.max_val - sl.min_val);
        for (const auto& n : current_graph->nodes) {
            if (n.id == sl.node_id && n.desc && n.desc->set_scalar_in)
                n.desc->set_scalar_in(n.data, sl.port_name.c_str(),
                                      static_cast<double>(sl.value));
        }
        any_changed = true;
    }

    if (any_changed) {
        std::string json = serialize_graph(*current_graph);
        return GraphEdit{std::move(json)};
    }
    return std::nullopt;
}

// ── dwell delete ─────────────────────────────────────────────────────────────

std::optional<VrEditor::GraphEdit> VrEditor::update_dwell(
        const XrPosef* right_pose, bool grip_right, float dt, const Graph* current_graph) {
    if (!right_pose || !current_graph) return std::nullopt;
    // Deletion must be deliberate: bare hover deleted whatever you looked
    // at for 1 s. Require grip held, and never delete mid wire-drag.
    if (!grip_right || drag_state_ == DragState::Dragging) {
        dwell_s_ = 0.f;
        dwell_card_idx_ = -1;
        dwell_edge_idx_ = -1;
        return std::nullopt;
    }

    // Find which card (if any) the ray hits
    int hit_card = -1;
    auto cards_hit = RaySelector::test(*right_pose, node_cards_);
    if (cards_hit) hit_card = cards_hit->panel_index;

    // Find which edge the controller tip is nearest to (midpoint heuristic)
    int hit_edge = -1;
    float best_dist = 0.04f;
    for (size_t ei = 0; ei < current_edges_.size(); ++ei) {
        const auto& e = current_edges_[ei];
        Eigen::Vector3f mp{};
        bool found = false;
        for (size_t ci = 0; ci < node_cards_.size(); ++ci) {
            Eigen::Vector3f fp{}, tp{};
            bool hf = false, ht = false;
            if (card_ids_[ci] == e.from_node)
                for (const auto& h : output_handles_[ci])
                    if (h.port_name == e.from_port) { fp = h.world_pos; hf = true; break; }
            if (card_ids_[ci] == e.to_node)
                for (const auto& h : input_handles_[ci])
                    if (h.port_name == e.to_port) { tp = h.world_pos; ht = true; break; }
            if (hf && ht) { mp = (fp + tp) * 0.5f; found = true; break; }
        }
        if (!found) continue;
        float d = (controller_tip_ - mp).norm();
        if (d < best_dist) { best_dist = d; hit_edge = static_cast<int>(ei); }
    }

    // Dwell accumulation
    if (hit_card == dwell_card_idx_ && hit_edge == dwell_edge_idx_ &&
        (hit_card >= 0 || hit_edge >= 0)) {
        dwell_s_ += dt;
    } else {
        dwell_s_        = 0.f;
        dwell_card_idx_ = hit_card;
        dwell_edge_idx_ = hit_edge;
    }

    if (dwell_s_ >= 1.0f) {
        dwell_s_ = 0.f;
        std::string cur_json = serialize_graph(*current_graph);
        undo_json_ = cur_json;

        if (dwell_card_idx_ >= 0) {
            const std::string& nid = card_ids_[static_cast<size_t>(dwell_card_idx_)];
            dwell_card_idx_ = -1;
            return GraphEdit{remove_node(cur_json, nid)};
        }
        if (dwell_edge_idx_ >= 0) {
            size_t ei = static_cast<size_t>(dwell_edge_idx_);
            dwell_edge_idx_ = -1;
            return GraphEdit{remove_edge_at(cur_json, ei, current_edges_)};
        }
    }
    return std::nullopt;
}

// ── undo ─────────────────────────────────────────────────────────────────────

std::optional<VrEditor::GraphEdit> VrEditor::update_undo(
        const Eigen::Vector2f& thumb) {
    bool gesture = thumb.x() < -0.7f;
    bool rising  = gesture && !prev_undo_gesture_;
    prev_undo_gesture_ = gesture;
    if (rising && !undo_json_.empty()) {
        std::string json = std::move(undo_json_);
        undo_json_.clear();
        return GraphEdit{std::move(json)};
    }
    return std::nullopt;
}
