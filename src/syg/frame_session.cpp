// syg frame (PKG-4.3): render a graph headless to raw RGBA8 frames, one per
// head-chain completion, with ops applied per frame. Bytes to stdout, the
// timing stat to stderr (render-graph's discipline, so byte checks stay
// clean). Determinism class approximate — the test asserts golden-frame.md
// PROPERTIES, never bytes.
#include "frame_session.hpp"

#include <chrono>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "executor/exec_plan.hpp"
#include "parser/parser.hpp"
#include "render/render_region/render_region.hpp"

namespace syg::harness {

namespace {
std::string op_value(const nlohmann::json& op) {
  if (!op.contains("value")) return "";
  const auto& v = op.at("value");
  return v.is_string() ? v.get<std::string>() : v.dump();
}
}  // namespace

int frame_session(const nlohmann::json& in) {
  syg::executor::exec_plan p(syg::organs::parse_graph(in.at("graph")), 48000,
                             128);
  const auto& size = in.at("size");
  int w = size.at(0).get<int>(), h = size.at(1).get<int>();
  int frames = in.value("frames", 1);

  syg::render::render_region region;
  region.set_size(w, h);
  p.set_render_target(&region);

  // bind ops to a named local — pointers into a value() temporary dangle
  const auto ops = in.value("ops", nlohmann::json::array());
  std::map<int, std::vector<const nlohmann::json*>> by_frame;
  for (const auto& o : ops)
    by_frame[o.at("frame").get<int>()].push_back(&o);

  auto t0 = std::chrono::steady_clock::now();
  for (int k = 0; k < frames; ++k) {
    // apply this frame's ops at the boundary, then pump blocks until the
    // head chain completes exactly one more frame (the poll and the frame
    // tick fall on different blocks; a frame is ~6 blocks at 60/48k)
    for (const auto* op : by_frame[k]) {
      const std::string what = op->value("op", "set_param");
      if (what == "bang")
        p.post_event(op->at("route"), op->value("payload", 0.0));
      else if (what == "set_param")
        p.submit({what, op->at("route").get<std::string>(), op_value(*op),
                  op->value("author", "")});
      else
        p.submit({what, op->value("a", ""), op->value("b", ""),
                  op->value("author", "")});
    }
    int target = region.frames_rendered() + 1, guard = 0;
    while (region.frames_rendered() < target && guard++ < 4096) p.pump_block();
    const auto& px = region.rgba();
    std::fwrite(px.data(), 1, px.size(), stdout);
  }
  auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - t0)
                .count();
  std::fprintf(stderr, "{\"frames\": %d, \"ns_per_frame\": %ld}\n", frames,
               frames ? ns / frames : 0);
  return 0;
}

}  // namespace syg::harness
