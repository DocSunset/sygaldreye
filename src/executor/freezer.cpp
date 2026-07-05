// clause: scaffolding (dissolves: FRZ-1.1) — SHELVED rung-8 WIP: the
// codegen backend must fuse the REAL engine's realize output once CMP-9
// lands; this standalone walk is parked, not an endpoint
#include "freezer.hpp"

#include <fstream>
#include <map>
#include <set>
#include <stdexcept>

#include "lift.hpp"
#include "regions.hpp"
#include "subgraph/subgraph.hpp"

namespace syg::executor {
namespace {

std::optional<nlohmann::json> graphs_dir(const std::string& type) {
  std::ifstream f("graphs/" + type + ".json");
  if (!f) return std::nullopt;
  return nlohmann::json::parse(f);
}

// the freezer's per-native ledger: tier + codegen templates. New natives
// register here (the AUT-3 stamp will fold this into generation).
struct native_gen {
  int tier;                       // 1 freestanding, 2 platform-lib, 3 host
  const char* state;              // state decl template ($id)
  const char* per_sample;         // island/fused body ($id, $in*, $out)
};

const std::map<std::string, native_gen>& ledger() {
  static const std::map<std::string, native_gen> g{
      {"osc", {1,
               "float $id_phase = 0, $id_freq = 440, $id_off = 0;",
               "float $out = synth::sine($id_phase + $id_off);\n"
               "      $id_phase += 6.28318530f * $id_freq / 48000.0f;\n"
               "      if ($id_phase >= 6.28318530f) $id_phase -= 6.28318530f;"}},
      {"pulse", {1, "bool $id_fired = false;",
                 "float $out = $id_fired ? 0.0f : 1.0f;\n      $id_fired = true;"}},
      {"noise", {1, "uint32_t $id_rng = 0x1234567u;",
                 "float $out = synth::white_noise($id_rng);"}},
      {"add", {1, "", "float $out = $a + $b;"}},
      {"vca", {1, "float $id_gain = 0;", "float $out = $in * $id_gain;"}},
      {"delay", {1,
                 "synth::DelayLine $id_line; int $id_samples = 1;",
                 "float $out = $id_line.tick($in, $id_samples, 0.0f);"}},
      {"mix", {1, "", "float $out = $sum;"}},
      {"dac", {2, "", ""}},         // the platform boundary
      {"tmux", {3, "", ""}},        // host-bound (FRZ-2.1's culprit shape)
      {"lfo", {1,
               "float $id_phase = 0, $id_freq = 440, $id_depth = 1;",
               ""}},                // frame-side: emitted in the frame tick
      {"smoother", {1,
                    "float $id_cur = 0, $id_in = 0, $id_rate = 20;",
                    "$id_cur += std::clamp($id_in - $id_cur, "
                    "-$id_rate / 48000.0f, $id_rate / 48000.0f);\n"
                    "      float $out = $id_cur;"}},
  };
  return g;
}

std::string subst(std::string t, const std::string& id) {
  auto safe = id;
  for (auto& c : safe)
    if (c == '#' || c == '.') c = '_';
  for (std::size_t pos; (pos = t.find("$id")) != std::string::npos;)
    t.replace(pos, 3, "s_" + safe);
  return t;
}

}  // namespace

frozen freeze(const nlohmann::json& app_interchange, store::peer_store& s) {
  frozen out;
  auto app_cid = s.put_node(app_interchange, false);
  nlohmann::json recipe{{"op", "freeze"},
                        {"inputs", {{"app", {{"/", app_cid}}}}},
                        {"target", "tier-1"},
                        {"determinism", "exact"}};
  if (auto hit = s.memo_lookup(recipe)) {
    auto obj = s.get(hit->output);
    out.source.assign(obj->begin(), obj->end());
    out.artifact_cid = hit->output;
    out.provenance_cid = hit->provenance;
    out.memo = true;
    return out;
  }
  auto doc = organs::expand_subgraphs(
      lift_expand(organs::parse_graph(app_interchange), graphs_dir), graphs_dir);
  auto regions = infer_regions(doc);
  if (!regions.errors.empty())
    throw std::runtime_error("freeze: " + regions.errors.front());
  // tier: the max over the frozen closure (the sink boundary excluded)
  out.tier = 1;
  for (const auto& [id, type] : doc.nodes) {
    auto it = ledger().find(type);
    if (it == ledger().end())
      throw std::runtime_error("the freezer has no template for " + type);
    if (type != "dac" && it->second.tier > out.tier) {
      out.tier = it->second.tier;
      out.tier_culprit = id + " (" + type + ")";
    }
  }
  // emission: a fused movement mirroring the interpreter's segments —
  // per-sample bodies inside one block loop (islands legal by construction:
  // every edge reads this sample if the producer already ran, else last)
  std::string src =
      "// FROZEN by the sygaldreye freezer (ADR-014) — do not edit\n"
      "// provenance: app=" + app_cid + " target=tier-" +
      std::to_string(out.tier) + "\n"
      "#include <algorithm>\n#include <cstdint>\n"
      "#include \"synth_core/synth_core.hpp\"\n"
      "#include \"synth_core/kernels.hpp\"\n\n"
      "struct frozen_movement {\n";
  std::set<std::string> in_block(regions.block.begin(), regions.block.end());
  std::vector<std::pair<std::string, std::string>> blocks, frames;
  for (const auto& [id, type] : doc.nodes)
    (in_block.count(id) ? blocks : frames).emplace_back(id, type);
  for (const auto& [id, type] : doc.nodes) {
    auto st = subst(ledger().at(type).state, id);
    if (!st.empty()) src += "  " + st + "\n";
  }
  // defaults land in init()
  src += "  void init() {\n";
  for (const auto& [route, v] : doc.defaults.items()) {
    auto id = route.substr(0, route.find('/'));
    auto port = route.substr(route.find('/') + 1);
    auto safe = subst("$id", id);
    if (v.is_number())
      src += "    " + safe + "_" + port + " = " +
             std::to_string(v.get<double>()) + "f;\n";
    else if (v.is_string() && port == "shape" && v == "cosine")
      src += "    " + safe + "_off = 1.57079632679f;\n";
  }
  for (const auto& [id, type] : doc.nodes)
    if (type == "delay")
      src += "    " + subst("$id", id) + "_line.prepare(" +
             subst("$id", id) + "_samples + 1);\n";
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
         "    t += 128.0 / 48000.0;\n";
  // latches: frame cells land in params at the boundary
  for (const auto& m : regions.mappings)
    if (m.mapping == "latch") {
      const auto& [from, to] = doc.edges[m.edge];
      auto src_id = from.substr(0, from.find('/'));
      auto dst_id = to.substr(0, to.find('/'));
      auto dport = to.substr(to.find('/') + 1);
      src += "    " + subst("$id", dst_id) + "_" + dport + " = " +
             subst("$id", src_id) + "_cell;\n";
    }
  // the fused sample loop, in the INTERPRETER'S segment order (Tarjan SCC
  // + Kahn over the condensation, members in doc order) — the FRZ-1.1
  // byte-identity test binds the two implementations forever
  {
    std::map<std::string, std::size_t> bidx;
    for (std::size_t k = 0; k < blocks.size(); ++k) bidx[blocks[k].first] = k;
    auto dlen = [&](const std::string& id) {
      auto it = doc.defaults.find(id + "/samples");
      return it != doc.defaults.end() && it->is_number() ? it->get<double>()
                                                         : 0.0;
    };
    std::vector<std::vector<std::size_t>> adj(blocks.size());
    for (const auto& [f, t2] : doc.edges) {
      auto sid = f.substr(0, f.find('/'));
      auto did = t2.substr(0, t2.find('/'));
      if (!bidx.count(sid) || !bidx.count(did)) continue;
      if (blocks[bidx[did]].second == "delay" && dlen(did) >= 128) continue;
      adj[bidx[sid]].push_back(bidx[did]);
    }
    std::vector<int> comp(blocks.size(), -1), low(blocks.size()),
        num(blocks.size(), -1);
    std::vector<std::size_t> stk;
    std::vector<bool> on(blocks.size(), false);
    int counter = 0, ncomp = 0;
    auto dfs = [&](auto&& self, std::size_t v) -> void {
      num[v] = low[v] = counter++;
      stk.push_back(v);
      on[v] = true;
      for (auto w : adj[v]) {
        if (num[w] < 0) {
          self(self, w);
          low[v] = std::min(low[v], low[w]);
        } else if (on[w]) {
          low[v] = std::min(low[v], num[w]);
        }
      }
      if (low[v] == num[v]) {
        while (true) {
          auto w = stk.back();
          stk.pop_back();
          on[w] = false;
          comp[w] = ncomp;
          if (w == v) break;
        }
        ++ncomp;
      }
    };
    for (std::size_t v = 0; v < blocks.size(); ++v)
      if (num[v] < 0) dfs(dfs, v);
    std::vector<std::set<int>> cadj(static_cast<std::size_t>(ncomp));
    std::vector<int> indeg(static_cast<std::size_t>(ncomp), 0);
    for (const auto& [f, t2] : doc.edges) {
      auto sid = f.substr(0, f.find('/'));
      auto did = t2.substr(0, t2.find('/'));
      if (!bidx.count(sid) || !bidx.count(did)) continue;
      auto cs = comp[bidx[sid]], cd = comp[bidx[did]];
      if (cs != cd && cadj[static_cast<std::size_t>(cs)].insert(cd).second)
        ++indeg[static_cast<std::size_t>(cd)];
    }
    std::vector<std::vector<std::size_t>> members(static_cast<std::size_t>(ncomp));
    for (std::size_t v = 0; v < blocks.size(); ++v)
      members[static_cast<std::size_t>(comp[v])].push_back(v);
    std::vector<int> ready;
    for (int c = 0; c < ncomp; ++c)
      if (indeg[static_cast<std::size_t>(c)] == 0) ready.push_back(c);
    std::vector<std::pair<std::string, std::string>> ordered;
    while (!ready.empty()) {
      int c = ready.back();
      ready.pop_back();
      for (auto v : members[static_cast<std::size_t>(c)])
        ordered.push_back(blocks[v]);
      for (auto d : cadj[static_cast<std::size_t>(c)])
        if (--indeg[static_cast<std::size_t>(d)] == 0) ready.push_back(d);
    }
    blocks = std::move(ordered);
  }
  src += "    for (int i = 0; i < 128; ++i) {\n";
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
          return carried.count(f) ? "carry_" + subst("$id", f.substr(0, f.find('/')))
                                  : "v_" + subst("$id", f.substr(0, f.find('/')));
      return safe + "_" + port;  // unconnected: the param
    };
    if (type == "dac") {
      sink_expr = in_expr("in");
      continue;
    }
    auto body = subst(ledger().at(type).per_sample, id);
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
          sum += (sum.empty() ? "" : " + ") + ("v_" + subst("$id", f.substr(0, f.find('/'))));
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
  out.source = src;
  formats::byte_vec bytes(src.begin(), src.end());
  out.artifact_cid = s.put_raw(bytes, false);
  recipe["output"] = {{"/", out.artifact_cid}};
  auto c = s.commit_derivation(
      {{"op", "freeze"},
       {"inputs", {{"app", {{"/", app_cid}}}}},
       {"target", "tier-1"},
       {"determinism", "exact"}},
      {{"kind", "frozen-artifact"}, {"source", {{"/", out.artifact_cid}}}});
  out.provenance_cid = c.provenance;
  return out;
}

std::string unfreeze(const std::string& artifact_source) {
  auto pos = artifact_source.find("app=");
  if (pos == std::string::npos)
    throw std::runtime_error("no provenance in the artifact");
  auto end = artifact_source.find(' ', pos);
  return artifact_source.substr(pos + 4, end - pos - 4);
}

}  // namespace syg::executor
