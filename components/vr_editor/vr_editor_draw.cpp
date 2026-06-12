// Copyright 2025 Travis West
// VrEditor::draw() implementation
#include "vr_editor.hpp"
#include <GLES3/gl3.h>
#include <Eigen/Geometry>
#include <array>
#include <cstdio>

static constexpr int kBezierSegs = 14;

void draw_wire(const Eigen::Vector3f& p0, const Eigen::Vector3f& p3,
               const Eigen::Vector4f& color,
               const Eigen::Matrix4f& vp, RgbaShader& shader) {
    // P1 = p0 + forward 0.1, P2 = p3 - forward 0.1 (cards face +Z)
    Eigen::Vector3f p1 = p0 + Eigen::Vector3f{0, 0, -0.1f};
    Eigen::Vector3f p2 = p3 + Eigen::Vector3f{0, 0, -0.1f};

    std::array<float, (kBezierSegs+1)*3> pts{};
    for (int i = 0; i <= kBezierSegs; ++i) {
        float t  = static_cast<float>(i) / static_cast<float>(kBezierSegs);
        float t2 = t*t, t3 = t2*t;
        float u  = 1.f - t, u2 = u*u, u3 = u2*u;
        Eigen::Vector3f p = u3*p0 + 3.f*u2*t*p1 + 3.f*u*t2*p2 + t3*p3;
        pts[static_cast<size_t>(i)*3+0] = p.x();
        pts[static_cast<size_t>(i)*3+1] = p.y();
        pts[static_cast<size_t>(i)*3+2] = p.z();
    }

    GLuint vao=0, vbo=0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(pts.size()*sizeof(float)),
                 pts.data(), GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    shader.use();
    shader.set_mvp(vp);
    shader.set_color(color);
    glDrawArrays(GL_LINE_STRIP, 0, kBezierSegs+1);
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void VrEditor::draw(const Eigen::Matrix4f& vp, const TextMesh& text) const {
    // Palette: header row flips pages, then the current page's slice
    palette_panel_.draw(vp, shader_);
    char hdr[32];
    std::snprintf(hdr, sizeof(hdr), "more... %d/%d",
                  palette_page_ + 1, palette_pages());
    for (int r = 0; r <= kPaletteRows; ++r) {
        int idx = palette_page_ * kPaletteRows + r - 1;
        if (r > 0 && idx >= static_cast<int>(palette_types_.size())) break;
        float v = (static_cast<float>(r) + 0.5f) /
                  static_cast<float>(kPaletteRows + 1);
        float y = palette_panel_.position.y() + palette_panel_.height * (0.5f - v);
        Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
        m(0,0) = m(1,1) = 0.02f * text_scale;
        m(0,3) = palette_panel_.position.x() - palette_panel_.width * 0.45f;
        m(1,3) = y;
        m(2,3) = palette_panel_.position.z() + 0.002f;
        text.draw(r == 0 ? hdr : palette_types_[static_cast<size_t>(idx)].c_str(),
                  vp * m);
    }

    // Node cards
    for (int i = 0; i < static_cast<int>(node_cards_.size()); ++i) {
        auto si = static_cast<size_t>(i);
        node_cards_[si].draw(vp, shader_);
        Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
        m(0,0) = m(1,1) = 0.018f * text_scale;
        const auto& c = node_cards_[si];
        m(0,3) = c.position.x() - c.width * 0.45f;
        m(1,3) = c.position.y() + c.height * 0.4f;
        m(2,3) = c.position.z() + 0.002f;
        text.draw(card_ids_[si], vp * m);

        // Input handles
        for (const auto& h : input_handles_[si])
            draw_quad(h.world_pos, 0.006f, 0.006f,
                      port_kind_color(h.port_kind), vp, shader_);

        // Output handles
        for (const auto& h : output_handles_[si])
            draw_quad(h.world_pos, 0.006f, 0.006f,
                      port_kind_color(h.port_kind), vp, shader_);

        // Sliders
        for (const auto& sl : sliders_[si]) {
            draw_quad(sl.world_pos, sl.width*0.5f, sl.height*0.5f,
                      {0.2f, 0.2f, 0.2f, 1.0f}, vp, shader_);
            float norm = (sl.max_val > sl.min_val)
                ? (sl.value - sl.min_val) / (sl.max_val - sl.min_val) : 0.5f;
            float thumb_x = sl.world_pos.x() - sl.width*0.5f + norm * sl.width;
            // Hover feedback + a z offset so the thumb never z-fights the track.
            Eigen::Vector3f delta = controller_tip_ - sl.world_pos;
            bool hov = std::abs(delta.x()) < sl.width * 0.5f &&
                       std::abs(delta.y()) < 0.03f && std::abs(delta.z()) < 0.06f;
            draw_quad({thumb_x, sl.world_pos.y(), sl.world_pos.z() + 0.004f},
                      hov ? 0.006f : 0.004f, sl.height * (hov ? 0.85f : 0.6f),
                      hov ? Eigen::Vector4f{1.0f, 0.85f, 0.3f, 1.0f}
                          : Eigen::Vector4f{0.7f, 0.8f, 1.0f, 1.0f}, vp, shader_);
        }
    }

    // Wires between connected port handles
    for (const auto& edge : current_edges_) {
        Eigen::Vector3f from_pos{}, to_pos{};
        bool found_from = false, found_to = false;
        std::string edge_kind;

        for (size_t ci = 0; ci < node_cards_.size(); ++ci) {
            if (card_ids_[ci] == edge.from_node) {
                for (const auto& h : output_handles_[ci]) {
                    if (h.port_name == edge.from_port) {
                        from_pos = h.world_pos; found_from = true;
                        edge_kind = h.port_kind; break;
                    }
                }
            }
            if (card_ids_[ci] == edge.to_node) {
                for (const auto& h : input_handles_[ci]) {
                    if (h.port_name == edge.to_port) {
                        to_pos = h.world_pos; found_to = true; break;
                    }
                }
            }
        }

        if (found_from && found_to)
            draw_wire(from_pos, to_pos, port_kind_color(edge_kind), vp, shader_);
    }

    // Ghost wire during drag
    if (drag_state_ == DragState::Dragging)
        draw_wire(drag_from_pos_, controller_tip_,
                  port_kind_color(drag_from_kind_), vp, shader_);

    // ── presence: controller tip markers (the poke stick is the pointer) ───
    if (show_right_) {
        Eigen::Vector4f tip_col = grip_r_ ? Eigen::Vector4f{0.4f, 1.0f, 0.5f, 0.8f}
                       : trig_r_ ? Eigen::Vector4f{1.0f, 0.8f, 0.3f, 0.8f}
                                 : Eigen::Vector4f{0.6f, 0.7f, 1.0f, 0.5f};
        draw_quad(right_tip_, 0.008f, 0.008f, tip_col, vp, shader_);
    }
    if (show_left_)
        draw_quad(left_tip_, 0.008f, 0.008f,
                  trig_l_ ? Eigen::Vector4f{1.0f, 0.8f, 0.3f, 0.9f}
                          : Eigen::Vector4f{0.5f, 0.9f, 0.9f, 0.6f}, vp, shader_);

    // ── hover label: legible name beside whatever the tip is near ──────────
    if (!hover_label_.empty()) {
        Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
        m(0,0) = m(1,1) = 0.014f * text_scale;
        m(0,3) = hover_pos_.x() + 0.018f;
        m(1,3) = hover_pos_.y() + 0.016f;
        m(2,3) = hover_pos_.z() + 0.012f;
        // backdrop so it reads against any scene
        draw_quad({hover_pos_.x() + 0.06f, hover_pos_.y() + 0.02f, hover_pos_.z() + 0.010f},
                  0.06f, 0.014f, {0.02f, 0.02f, 0.05f, 0.92f}, vp, shader_);
        text.draw(hover_label_, vp * m);
    }
}
