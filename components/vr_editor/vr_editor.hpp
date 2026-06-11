// Copyright 2025 Travis West
#pragma once
#include "vr_panel.hpp"
#include "rgba_shader.hpp"
#include "signal_graph.hpp"
#include "component_registry.hpp"
#include "text_mesh.hpp"
#include "port_schema_reader.hpp"
#include <openxr/openxr.h>
#include <Eigen/Core>
#include <optional>
#include <unordered_map>
#include <string>
#include <vector>

struct PortHandle {
    std::string      node_id;
    std::string      port_name;
    std::string      port_kind;
    bool             is_output;
    Eigen::Vector3f  world_pos;
};

struct SliderWidget {
    std::string      node_id;
    std::string      port_name;
    Eigen::Vector3f  world_pos;
    float            width    = 0.08f;
    float            height   = 0.012f;
    float            value    = 0.f;
    float            min_val  = 0.f;
    float            max_val  = 1.f;
    bool             active   = false;
};

// Color by port kind (shared between draw files)
Eigen::Vector4f port_kind_color(const std::string& kind);

// Draw a small quad at world_pos
void draw_quad(const Eigen::Vector3f& pos, float hw, float hh,
               const Eigen::Vector4f& color,
               const Eigen::Matrix4f& vp, RgbaShader& shader);

// Draw a cubic bezier as line segments
void draw_wire(const Eigen::Vector3f& p0, const Eigen::Vector3f& p3,
               const Eigen::Vector4f& color,
               const Eigen::Matrix4f& vp, RgbaShader& shader);

struct VrEditor {
    void init(const ComponentRegistry& registry, const Graph* graph);

    struct GraphEdit { std::string new_graph_json; };

    std::optional<GraphEdit> update(const XrPosef* left_pose,
                                    const XrPosef* right_pose,
                                    bool trigger_left,
                                    bool trigger_right,
                                    bool grip_right,
                                    const Eigen::Vector2f& thumbstick_left,
                                    float delta_time_s,
                                    const Graph* current_graph,
                                    const ComponentRegistry& registry);

    void draw(const Eigen::Matrix4f& vp, const TextMesh& text) const;

    void on_graph_changed(const Graph* graph);

// private data — accessed from multiple .cpp files in the same library
    mutable RgbaShader   shader_;
    VrPanel              palette_panel_;
    std::vector<VrPanel> node_cards_;
    std::vector<std::string> palette_types_;
    std::vector<std::string> card_ids_;
    bool prev_trigger_right_ = false;

    std::vector<std::vector<PortHandle>> input_handles_;
    std::vector<std::vector<PortHandle>> output_handles_;
    std::vector<std::vector<SliderWidget>> sliders_;
    std::vector<Edge> current_edges_;
    std::string undo_json_;

    enum class DragState { Idle, Dragging, MovingCard } drag_state_ = DragState::Idle;
    std::string drag_from_node_, drag_from_port_, drag_from_kind_;
    Eigen::Vector3f drag_from_pos_{};
    bool prev_grip_right_ = false;
    Eigen::Vector3f controller_tip_{};

    // presence + hover feedback (drawn every frame)
    Eigen::Vector3f left_tip_{0, -10, 0}, right_tip_{0, -10, 0};
    Eigen::Vector3f ray_origin_{}, ray_dir_{0, 0, -1};
    bool show_left_ = false, show_right_ = false;
    bool trig_l_ = false, trig_r_ = false, grip_r_ = false;
    std::string     hover_label_;
    Eigen::Vector3f hover_pos_{};

    // movable cards: per-node position overrides (in-memory; persistence
    // arrives with instanced_by metadata — see kanban)
    std::unordered_map<std::string, Eigen::Vector3f> card_pos_ovr_;
    int             moving_card_ = -1;
    Eigen::Vector3f move_grab_offset_{};
    void rebuild_card(std::size_t idx, const ComponentRegistry* reg = nullptr);

    float dwell_s_        = 0.f;
    int   dwell_card_idx_ = -1;
    int   dwell_edge_idx_ = -1;

    bool prev_undo_gesture_ = false;

    // sub-update helpers
    std::optional<GraphEdit> update_drag(bool grip_right, const Graph*);
    std::optional<GraphEdit> update_sliders(const XrPosef*, bool trigger, const Graph*);
    std::optional<GraphEdit> update_dwell(const XrPosef*, bool grip_right, float dt, const Graph*);
    std::optional<GraphEdit> update_undo(const Eigen::Vector2f& thumb);
};
