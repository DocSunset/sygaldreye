// Copyright 2026 Travis West
#include "editor_node.hpp"

namespace {
XrPosef to_pose(const Eigen::Vector3f& p, const Eigen::Quaternionf& q_in) {
    Eigen::Quaternionf q = (q_in.norm() > 1e-6f) ? q_in.normalized()
                                                 : Eigen::Quaternionf::Identity();
    return {{q.x(), q.y(), q.z(), q.w()}, {p.x(), p.y(), p.z()}};
}
} // namespace

void EditorNode::operator()(double time_s) {
    if (!graph_ || !registry_) return;  // platform hasn't injected context yet
    if (!ready_) {
        text_.init();
        editor_.init(*registry_, graph_);
        ready_ = true;
    }

    XrPosef lp = to_pose(inputs.left_pos.value,  inputs.left_rot.value);
    XrPosef rp = to_pose(inputs.right_pos.value, inputs.right_rot.value);
    float dt = (prev_time_s_ > 0.0) ? float(time_s - prev_time_s_) : 0.016f;
    prev_time_s_ = time_s;

    auto edit = editor_.update(&lp, &rp,
                               inputs.trigger_left.value  > 0.5f,
                               inputs.trigger_right.value > 0.5f,
                               inputs.grip_right.value    > 0.5f,
                               {inputs.thumb_x.value, inputs.thumb_y.value},
                               dt, graph_, *registry_);
    if (edit) pending_edit_ = std::move(edit->new_graph_json);

    outputs.render.value = [this](const Eigen::Matrix4f& vp) {
        editor_.draw(vp, text_);
    };
}
