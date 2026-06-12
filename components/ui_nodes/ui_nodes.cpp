// Copyright 2026 Travis West
#include "ui_nodes.hpp"
#include <algorithm>

namespace {

Eigen::Vector3f ray_dir(const Eigen::Quaternionf& q_in) {
    Eigen::Quaternionf q = (q_in.norm() > 1e-6f) ? q_in.normalized()
                                                 : Eigen::Quaternionf::Identity();
    return q * Eigen::Vector3f{0.f, 0.f, -1.f};
}

VrPanel make_panel(float x, float y, float z, float w, float h,
                   const Eigen::Vector4f& color) {
    VrPanel p;
    p.position = {x, y, z};
    p.width    = w;
    p.height   = h;
    p.color    = color;
    return p;
}

} // namespace

void UiSliderNode::operator()(double) {
    float lo = endpoints.min.get(), hi = endpoints.max.get();
    if (value_norm_ < 0.f) {
        float span = hi - lo;
        value_norm_ = (span != 0.f)
            ? std::clamp((endpoints.init.get() - lo) / span, 0.f, 1.f)
            : 0.f;
    }

    VrPanel track = make_panel(endpoints.x.get(), endpoints.y.get(), endpoints.z.get(),
                               endpoints.width.get(), 0.03f,
                               {0.15f, 0.17f, 0.22f, 0.95f});
    auto hit = track.intersect(endpoints.ray_pos.get(), ray_dir(endpoints.ray_rot.get()));
    hover_ = hit.has_value();
    if (hover_ && endpoints.trigger.get() > 0.5f)
        value_norm_ = std::clamp(hit->uv.x(), 0.f, 1.f);

    endpoints.value.value = lo + value_norm_ * (hi - lo);

    endpoints.render.value = [this, track](const Eigen::Matrix4f& vp) {
        if (!shader_ready_) { shader_.create(); shader_ready_ = true; }
        track.draw(vp, shader_);
        VrPanel thumb = track;
        thumb.position.x() += (value_norm_ - 0.5f) * track.width;
        thumb.position.z() += 0.005f;
        thumb.width  = 0.02f;
        thumb.height = 0.05f;
        thumb.color  = hover_ ? Eigen::Vector4f{0.6f, 0.9f, 1.f, 1.f}
                              : Eigen::Vector4f{0.4f, 0.6f, 0.9f, 1.f};
        thumb.draw(vp, shader_);
    };
}

void UiButtonNode::operator()(double) {
    VrPanel face = make_panel(endpoints.x.get(), endpoints.y.get(), endpoints.z.get(),
                              endpoints.width.get(), endpoints.height.get(),
                              {0.2f, 0.25f, 0.3f, 0.95f});
    auto hit = face.intersect(endpoints.ray_pos.get(), ray_dir(endpoints.ray_rot.get()));
    hover_   = hit.has_value();
    pressed_ = hover_ && endpoints.trigger.get() > 0.5f;
    endpoints.pressed.value = pressed_ ? 1.f : 0.f;
    endpoints.hover.value   = hover_ ? 1.f : 0.f;

    endpoints.render.value = [this, face](const Eigen::Matrix4f& vp) {
        if (!shader_ready_) { shader_.create(); shader_ready_ = true; }
        VrPanel f = face;
        if (pressed_)     f.color = {0.9f, 0.6f, 0.2f, 1.f};
        else if (hover_)  f.color = {0.35f, 0.45f, 0.55f, 1.f};
        f.draw(vp, shader_);
    };
}

void UiPaneNode::operator()(double) {
    VrPanel pane = make_panel(endpoints.x.get(), endpoints.y.get(), endpoints.z.get(),
                              endpoints.width.get(), endpoints.height.get(),
                              {endpoints.r.get(), endpoints.g.get(), endpoints.b.get(),
                               endpoints.alpha.get()});
    endpoints.render.value = [this, pane](const Eigen::Matrix4f& vp) {
        if (!shader_ready_) { shader_.create(); shader_ready_ = true; }
        pane.draw(vp, shader_);
    };
}
