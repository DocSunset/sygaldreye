// syg shell (PKG-4.4): the host shell IS the ordinary peer — a booted plan
// whose arbiter takes gesture edits, with the render executor presenting
// (offscreen under test). Pointer input reaches the graph ONLY through the
// pointer source node (N4); its click bangs op_buttons whose ops land in the
// graph's own arbiter (self-edit — inject_context self-points it). No HTTP
// side door (MSH-2): the interactive/agent path is the mesh (Phase B.2).
#include "shell_session.hpp"

#include <cstdio>
#include <string>

#include "executor/exec_plan.hpp"
#include "parser/parser.hpp"
#include "render/render_region/render_region.hpp"

namespace syg::harness {

int shell_session(const nlohmann::json& in) {
  syg::executor::exec_plan p(syg::organs::parse_graph(in.at("graph")), 48000,
                             128);
  const auto& size = in.at("size");
  int w = size.at(0).get<int>(), h = size.at(1).get<int>();
  syg::render::render_region region;
  region.set_size(w, h);
  p.set_render_target(&region);

  // the one pointer source the window/script feeds (one pointer per shell)
  std::string ptr;
  for (const auto& [id, type] : p.doc().nodes)
    if (type == "pointer") { ptr = id; break; }

  const auto script = in.at("script");
  for (const auto& step : script) {
    if (step.contains("pointer")) {
      const auto& pt = step.at("pointer");
      auto feed = [&](const char* port, const char* key) {
        if (pt.contains(key) && !ptr.empty())
          p.submit({"set_param", ptr + "/" + port,
                    std::to_string(pt.at(key).get<double>()), "pointer"});
      };
      feed("x", "x");
      feed("y", "y");
      feed("buttons", "buttons");
    } else if (step.contains("settle")) {
      // pump enough blocks (~5 frame periods) for the click to cross the
      // chain into op events, submit to the arbiter, and apply next boundary
      for (int i = 0; i < 32; ++i) p.pump_block();
    } else if (step.contains("frame")) {
      int target = region.frames_rendered() + 1, guard = 0;
      while (region.frames_rendered() < target && guard++ < 4096)
        p.pump_block();
      const auto& px = region.rgba();
      std::fwrite(px.data(), 1, px.size(), stdout);
    } else if (step.contains("doc")) {
      auto s = syg::organs::serialize_graph(p.doc()).dump();
      std::fwrite(s.data(), 1, s.size(), stdout);
    }
  }
  return 0;
}

}  // namespace syg::harness
