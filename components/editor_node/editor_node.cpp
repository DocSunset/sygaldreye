// Copyright 2026 Travis West
#include "editor_node.hpp"

namespace {
XrPosef to_pose(const Eigen::Vector3f& p, const Eigen::Quaternionf& q_in) {
    Eigen::Quaternionf q =
        (q_in.norm() > 1e-6f) ? q_in.normalized() : Eigen::Quaternionf::Identity();
    return {{q.x(), q.y(), q.z(), q.w()}, {p.x(), p.y(), p.z()}};
}
}  // namespace

void EditorNode::operator()(double time_s) {
    if (!graph_ || !registry_) return;  // platform hasn't injected context yet
    if (!ready_) {
        text_.init();
        editor_.init(*registry_, graph_);
        ready_ = true;
    } else if (graph_ != last_graph_) {
        // Migration keeps this node alive across swaps; the editor's card
        // model must follow the new graph or every interaction targets
        // stale node ids.
        editor_.on_graph_changed(graph_);
    }
    last_graph_ = graph_;

    XrPosef lp = to_pose(endpoints.left_pos.get(), endpoints.left_rot.get());
    XrPosef rp = to_pose(endpoints.right_pos.get(), endpoints.right_rot.get());
    float dt = (prev_time_s_ > 0.0) ? float(time_s - prev_time_s_) : 0.016f;
    prev_time_s_ = time_s;
    editor_.text_scale = endpoints.text_scale.get();

    auto edit = editor_.update(
        &lp,
        &rp,
        endpoints.trigger_left.get() > 0.5f,
        endpoints.trigger_right.get() > 0.5f,
        endpoints.grip_right.get() > 0.5f,
        {endpoints.thumb_x.get(), endpoints.thumb_y.get()},
        dt,
        graph_,
        *registry_);
    if (edit && edits_) edits_->push(std::move(edit->new_graph_json));

    editor_.collect_wires(wire_rows_);
    endpoints.wires.value = Span{wire_rows_.data(), int(wire_rows_.size() / 10), 10};

    endpoints.render.value = [this](const Eigen::Matrix4f& vp) { editor_.draw(vp, text_); };
}
