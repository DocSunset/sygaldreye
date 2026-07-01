// Copyright 2026 Travis West
#include "palette.hpp"

#include <cmath>

void PaletteNode::operator()(double) {
    bool trig = endpoints.trigger.get() > 0.5f;
    bool fire = trig && !prev_trigger_;
    prev_trigger_ = trig;
    if (!fire || !ctx_.types || ctx_.types->empty()) return;

    Eigen::Vector3f tip = editor_layout::controller_tip(endpoints.pos.get(), endpoints.rot.get());
    Eigen::Vector3f d = tip - panel_pos();
    if (std::abs(d.x()) > panel_w() * 0.5f || std::abs(d.y()) > panel_h() * 0.5f ||
        std::abs(d.z()) > 0.08f)
        return;

    int row = int((0.5f - d.y() / panel_h()) * float(kRows + 1));
    if (row <= 0) {  // header row flips pages
        page_ = (page_ + 1) % pages();
        return;
    }
    int idx = page_ * kRows + row - 1;
    if (idx < 0 || idx >= int(ctx_.types->size())) return;

    if (ctx_.edits)
        ctx_.edits->push(
            "{\"op\":\"add_node\",\"type\":\"" + (*ctx_.types)[std::size_t(idx)] + "\"}");
}
