// clause: machinery — realize's codegen backend: emission (ADR-014)
#include "codegen.hpp"

#include <map>
#include <set>
#include <stdexcept>

#include "regions.hpp"

namespace syg::executor {
namespace {

std::string subst(std::string t, const std::string& id) {
  auto safe = id;
  for (auto& c : safe)
    if (c == '#' || c == '.') c = '_';
  for (std::size_t pos; (pos = t.find("$id")) != std::string::npos;)
    t.replace(pos, 3, "s_" + safe);
  return t;
}

}  // namespace

emitted emit_frozen(const organs::graph_doc& doc,
                    const nlohmann::json& ledger, int block) {
  emitted out;
  auto tmpl = [&](const std::string& type) -> const nlohmann::json& {
    auto it = ledger.find(type);
    if (it == ledger.end())
      throw std::runtime_error("the codegen ledger has no entry for " + type);
    return *it;
  };
  // tier: the max over the frozen closure (the sink boundary excluded)
  for (const auto& [id, type] : doc.nodes) {
    if (id.starts_with("__")) continue;
    int t = tmpl(type).value("tier", 1);
    if (type != "dac" && t > out.tier) {
      out.tier = t;
      out.tier_culprit = id + " (" + type + ")";
    }
  }
  std::set<std::string> in_block;
  for (const auto& b : doc.defaults.value("__regions", nlohmann::ordered_json::object())
                           .value("block", nlohmann::json::array()))
    in_block.insert(b.get<std::string>());
  std::vector<std::pair<std::string, std::string>> blocks, frames;
  for (const auto& [id, type] : doc.nodes) {
    if (id.starts_with("__")) continue;
    (in_block.count(id) ? blocks : frames).emplace_back(id, type);
  }
  // the artifact is SELF-CONTAINED (FRZ-3: closure = libc math only) —
  // kernel math copied verbatim from synth_core (AUT-1's floor), the
  // delay's capacity BAKED at freeze time (no heap, no <vector>)
  std::string src =
      "// FROZEN by realize's codegen backend (ADR-014) — do not edit;\n"
      "// provenance lives in the store (unfreeze = read it)\n"
      "#include <stdint.h>\n"
      "extern \"C\" float sinf(float);  // newlib; <math.h> is hosted-only C++\n\n"
      "namespace synth {\n"
      "inline float sine(float phase) { return sinf(phase); }\n"
      "inline float white_noise(uint32_t& state) {\n"
      "  state ^= state << 13;\n"
      "  state ^= state >> 17;\n"
      "  state ^= state << 5;\n"
      "  return (static_cast<float>(state) / float(0x80000000u)) - 1.0f;\n"
      "}\n"
      "inline float clampf(float v, float lo, float hi) {\n"
      "  return v < lo ? lo : (hi < v ? hi : v);\n"
      "}\n"
      "template <int CAP>\n"
      "struct DelayLine {  // frozen form: capacity baked, math identical\n"
      "  float buf[CAP] = {};\n"
      "  int write = 0;\n"
      "  float tick(float dry, int delay_samples, float feedback) {\n"
      "    int len = CAP;\n"
      "    int rd = (write - delay_samples + len) % len;\n"
      "    float echo = buf[rd];\n"
      "    buf[write] = dry + echo * feedback;\n"
      "    write = (write + 1) % len;\n"
      "    return echo;\n"
      "  }\n"
      "};\n"
      "}  // namespace synth\n\n"
      "struct frozen_movement {\n";
  auto baked_cap = [&](const std::string& id) {
    auto it = doc.defaults.find(id + "/samples");
    double n = it != doc.defaults.end() && it->is_number() ? it->get<double>()
                                                           : 1.0;
    return std::to_string(static_cast<long>(n) + 1);
  };
  for (const auto& [id, type] : doc.nodes) {
    if (id.starts_with("__")) continue;
    auto st = subst(tmpl(type).value("state", ""), id);
    for (std::size_t pos; (pos = st.find("$cap")) != std::string::npos;)
      st.replace(pos, 4, baked_cap(id));
    if (!st.empty()) src += "  " + st + "\n";
  }
  // defaults land in init()
  src += "  void init() {\n";
  for (const auto& [route, v] : doc.defaults.items()) {
    if (route.starts_with("__")) continue;
    auto id = route.substr(0, route.find('/'));
    auto port = route.substr(route.find('/') + 1);
    auto safe = subst("$id", id);
    if (v.is_number())
      src += "    " + safe + "_" + port + " = " +
             std::to_string(v.get<double>()) + "f;\n";
    else if (v.is_string() && port == "shape" && v == "cosine")
      src += "    " + safe + "_off = 1.57079632679f;\n";
  }
  src += "  }\n";
  // frame state cells + the pump
  src += "  double t = 0, next_frame = 0;\n";
  for (const auto& [id, t2] : frames)
    src += "  float " + subst("$id", id) + "_cell = 0;\n";
  src += "  void pump_block(float* out) {\n"
         "    if (t >= next_frame) {\n";
  for (const auto& [id, type] : frames)
    if (type == "lfo") {
      auto p = subst("$id", id);
      src += "      " + p + "_phase += 6.28318530f * " + p +
             "_freq * (1.0f / 60.0f);\n"
             "      if (" + p + "_phase >= 6.28318530f) " + p +
             "_phase -= 6.28318530f;\n"
             "      " + p + "_cell = " + p + "_depth * 0.5f * (1.0f + "
             "synth::sine(" + p + "_phase));\n";
    }
  src += "      next_frame += 1.0 / 60.0;\n    }\n"
         "    t += " + std::to_string(block) + ".0 / 48000.0;\n";
  // latches: frame cells land in params at the boundary
  for (const auto& m : doc.defaults.value("__mappings", nlohmann::ordered_json::array())) {
    if (m.value("mapping", "") != "latch") continue;
    const auto& [from, to] = doc.edges.at(m.at("edge").get<std::size_t>());
    auto src_id = from.substr(0, from.find('/'));
    auto dst_id = to.substr(0, to.find('/'));
    auto dport = to.substr(to.find('/') + 1);
    src += "    " + subst("$id", dst_id) + "_" + dport + " = " +
           subst("$id", src_id) + "_cell;\n";
  }
  // the fused sample loop in the INTERPRETER'S schedule — scc_order is the
  // one ordering (extracted from the segment builder, never re-derived)
  {
    std::map<std::string, std::size_t> bidx;
    for (std::size_t k = 0; k < blocks.size(); ++k) bidx[blocks[k].first] = k;
    auto dlen = [&](const std::string& id) {
      auto it = doc.defaults.find(id + "/samples");
      return it != doc.defaults.end() && it->is_number() ? it->get<double>()
                                                         : 0.0;
    };
    std::vector<std::pair<std::size_t, std::size_t>> cut_edges;
    for (const auto& [f, t2] : doc.edges) {
      auto sid = f.substr(0, f.find('/'));
      auto did = t2.substr(0, t2.find('/'));
      if (!bidx.count(sid) || !bidx.count(did)) continue;
      if (blocks[bidx[did]].second == "delay" && dlen(did) >= block) continue;
      cut_edges.emplace_back(bidx[sid], bidx[did]);
    }
    auto sched = scc_order(blocks.size(), cut_edges);
    std::vector<std::pair<std::string, std::string>> ordered;
    for (const auto& comp : sched.components)
      for (auto v : comp) ordered.push_back(blocks[v]);
    blocks = std::move(ordered);
  }
  src += "    for (int i = 0; i < " + std::to_string(block) + "; ++i) {\n";
  std::map<std::string, std::size_t> order;
  for (std::size_t k = 0; k < blocks.size(); ++k) order[blocks[k].first] = k;
  std::set<std::string> carried;  // "src/port" values carried across samples
  for (const auto& [f, t2] : doc.edges) {
    auto sid = f.substr(0, f.find('/'));
    auto did = t2.substr(0, t2.find('/'));
    if (order.count(sid) && order.count(did) && order[sid] >= order[did])
      carried.insert(f);
  }
  std::string sink_expr = "0.0f";
  for (const auto& [id, type] : blocks) {
    auto safe = subst("$id", id);
    auto in_expr = [&](const std::string& port) -> std::string {
      for (const auto& [f, t2] : doc.edges)
        if (t2 == id + "/" + port)
          return carried.count(f)
                     ? "carry_" + subst("$id", f.substr(0, f.find('/')))
                     : "v_" + subst("$id", f.substr(0, f.find('/')));
      return safe + "_" + port;  // unconnected: the param
    };
    if (type == "dac") {
      sink_expr = in_expr("in");
      continue;
    }
    std::string body = subst(tmpl(type).value("per_sample", ""), id);
    auto replace = [&](const std::string& key, const std::string& val) {
      for (std::size_t pos; (pos = body.find(key)) != std::string::npos;)
        body.replace(pos, key.size(), val);
    };
    replace("$out", "v_" + safe);
    replace("$in", in_expr("in"));
    replace("$a", in_expr("a"));
    replace("$b", in_expr("b"));
    if (type == "mix") {
      std::string sum;
      for (const auto& [f, t2] : doc.edges)
        if (t2 == id + "/in")
          sum += (sum.empty() ? "" : " + ") +
                 ("v_" + subst("$id", f.substr(0, f.find('/'))));
      replace("$sum", sum.empty() ? "0.0f" : sum);
    }
    src += "      " + body + "\n";
  }
  src += "      out[i] = " + sink_expr + ";\n";
  for (const auto& f : carried)
    src += "      carry_" + subst("$id", f.substr(0, f.find('/'))) + " = v_" +
           subst("$id", f.substr(0, f.find('/'))) + ";\n";
  src += "    }\n  }\n";
  for (const auto& f : carried)
    src += "  float carry_" + subst("$id", f.substr(0, f.find('/'))) + " = 0;\n";
  src += "};\n";
  // the hosted plugin gate (dlopen) — omitted from freestanding builds so
  // tier-1 artifacts stay OS-symbol-free (FRZ-3)
  src +=
      "\n#ifndef SYG_FREESTANDING\n"
      "extern \"C\" {\n"
      "void* frozen_create() {\n"
      "  auto* m = new frozen_movement();\n"
      "  m->init();\n"
      "  return m;\n"
      "}\n"
      "void frozen_pump(void* p, float* out) {\n"
      "  static_cast<frozen_movement*>(p)->pump_block(out);\n"
      "}\n"
      "void frozen_destroy(void* p) { delete static_cast<frozen_movement*>(p); }\n"
      "}\n"
      "#endif\n";
  out.source = std::move(src);
  return out;
}

}  // namespace syg::executor
