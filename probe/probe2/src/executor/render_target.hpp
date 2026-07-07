// clause: floor — the render seam. exec_plan drives a render_target at the
// head-chain boundary; the render package's render_region implements it with
// real GL. This header names NO GL (it is the executor's side of the seam),
// so it carries no pkg41 tokens; only render_region-named files do.
#pragma once

#include <vector>

#include "crown.hpp"

namespace syg::executor {

// The device the frame region presents to (the graphics analogue of the
// audio dac sink). Nodes stay passive — the executor reads each on-chain
// draw's held mesh+surface values and hands them here, in head-chain order,
// bracketed by begin/end. Headless renders read back through rgba().
struct render_target {
  virtual ~render_target() = default;
  virtual void set_size(int w, int h) = 0;
  virtual void begin_frame() = 0;
  virtual void draw_one(const crown::svalue& mesh,
                        const crown::svalue& surface) = 0;
  virtual void end_frame() = 0;
  virtual int frames_rendered() const = 0;
  virtual const std::vector<unsigned char>& rgba() const = 0;
};

}  // namespace syg::executor
