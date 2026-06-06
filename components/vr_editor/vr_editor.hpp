#pragma once
#include "vr_panel.hpp"
#include "rgba_shader.hpp"
#include "signal_graph.hpp"
#include "component_registry.hpp"
#include "text_mesh.hpp"
#include <openxr/openxr.h>
#include <Eigen/Core>
#include <optional>
#include <string>
#include <vector>

struct VrEditor {
    // init must be called once after GL context is current.
    void init(const ComponentRegistry& registry, const Graph* graph);

    struct GraphEdit { std::string new_graph_json; };

    std::optional<GraphEdit> update(const XrPosef* left_pose,
                                    const XrPosef* right_pose,
                                    bool trigger_left,
                                    bool trigger_right,
                                    const Graph* current_graph,
                                    const ComponentRegistry& registry);

    void draw(const Eigen::Matrix4f& vp, const TextMesh& text) const;

    void on_graph_changed(const Graph* graph);

private:
    mutable RgbaShader   shader_;
    VrPanel              palette_panel_;
    std::vector<VrPanel> node_cards_;
    std::vector<std::string> palette_types_;
    std::vector<std::string> card_ids_;
    bool prev_trigger_right_ = false;
};
