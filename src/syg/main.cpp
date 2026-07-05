#include <cstdio>
#include <cmath>
#include <map>
#include <thread>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include "address/address.hpp"
#include "chunk/chunk.hpp"
#include "cid/cid.hpp"
#include "pins/pins.hpp"
#include "dagcbor/dagcbor.hpp"
#include "codecs.hpp"
#include "abi_audits.hpp"
#include "crown.hpp"
#include "registry_face/registry_face.hpp"
#include "slot/slot.hpp"
#include "stage0_audits.hpp"
#include "naming_session.hpp"
#include "parser/parser.hpp"
#include "exec_plan.hpp"
#include "phase.hpp"
#include "regions.hpp"
#include "resolver/naive_resolver.hpp"
#include "hello_cosine/hello_cosine.hpp"
#include "ks/ks.hpp"
#include "oracle/oracle.hpp"

namespace {

std::string read_stdin() {
  std::ostringstream ss;
  ss << std::cin.rdbuf();
  return ss.str();
}

int cmd_encode() {
  auto v = nlohmann::json::parse(read_stdin());
  auto out = syg::formats::encode_projection(v);
  std::fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}

int cmd_hash() {
  auto in = read_stdin();
  syg::formats::byte_vec data(in.begin(), in.end());
  std::cout << syg::formats::cid_to_text(
                   syg::formats::cid_of(syg::formats::pins::multicodec_raw, data))
            << "\n";
  return 0;
}

int cmd_verify(const std::string& cid_text) {
  auto in = read_stdin();
  syg::formats::byte_vec data(in.begin(), in.end());
  bool ok = false;
  try {
    ok = syg::formats::cid_verify(syg::formats::cid_from_text(cid_text), data);
  } catch (const std::exception&) {
  }
  std::cout << (ok ? "{\"ok\":true}" : "{\"ok\":false}") << "\n";
  return 0;
}

int cmd_chunk_put() {
  auto in = nlohmann::json::parse(read_stdin());
  std::map<syg::formats::byte_vec, std::size_t> objects;  // cid -> size
  nlohmann::json roots = nlohmann::json::array();
  for (const auto& blob : in.at("blobs")) {
    auto root = syg::formats::chunk_put(
        syg::formats::bytes_of_projection(blob),
        [&](const syg::formats::byte_vec& cid, const syg::formats::byte_vec& obj) {
          objects.emplace(cid, obj.size());
        });
    roots.push_back(syg::formats::cid_to_text(root));
  }
  std::size_t stored = 0;
  for (const auto& [cid, size] : objects) stored += size;
  nlohmann::ordered_json out{
      {"roots", roots}, {"objects", objects.size()}, {"stored_bytes", stored}};
  std::cout << out.dump() << "\n";
  return 0;
}

int cmd_connection_legal() {
  auto in = nlohmann::json::parse(read_stdin());
  auto p = [&](const char* side) {
    return syg::naming::promise{in.at(side).at(0), in.at(side).at(1)};
  };
  auto v = syg::naming::connection_legal(p("from"), p("to"));
  nlohmann::json out{{"legal", v.legal},
                     {"mapping", v.mapping.empty()
                                     ? nlohmann::json(nullptr)
                                     : nlohmann::json(v.mapping)}};
  std::cout << out.dump() << "\n";
  return 0;
}

void stdout_sink(void*, const float* block, int n) {
  std::fwrite(block, sizeof(float), static_cast<std::size_t>(n), stdout);
}

int cmd_render_movement(const std::string& fixture, double seconds) {
  int frames = static_cast<int>(seconds * syg::movements::hello_cosine_rate);
  if (fixture == "hello-cosine")
    syg::movements::render_hello_cosine(frames, stdout_sink, nullptr);
  else if (fixture == "ks")
    syg::movements::render_ks(frames, stdout_sink, nullptr);
  else
    throw std::runtime_error("unknown movement fixture: " + fixture);
  return 0;
}

int cmd_tick_audit() {
  // NAM-5.4: tick the frozen movement with the naming instrumentation live.
  long before = syg::naming::lookup_count();
  int frames = syg::movements::hello_cosine_rate;  // one second of ticks
  syg::movements::render_hello_cosine(
      frames, [](void*, const float*, int) {}, nullptr);
  nlohmann::json out{{"ticks", frames},
                     {"lookups", syg::naming::lookup_count() - before}};
  std::cout << out.dump() << "\n";
  return 0;
}

int cmd_codec_selftest(const std::string& type) {
  auto j = nlohmann::json::parse(read_stdin());
  nlohmann::json out;
  if (type == "widget_a") {
    syg::nodes::widget_a x;
    syg::generated::from_projection(j, x);
    out = syg::generated::to_projection(x);
  } else if (type == "widget_b") {
    syg::nodes::widget_b x;
    syg::generated::from_projection(j, x);
    out = syg::generated::to_projection(x);
  } else {
    throw std::runtime_error("no generated codec for " + type);
  }
  std::cout << out.dump() << "\n";
  return 0;
}

syg::crown::plan replayed_plan(const std::string& tape_text, int block) {
  syg::crown::plan p(syg::organs::registered_natives(), block);
  for (auto& o : syg::crown::read_tape(tape_text)) p.submit(std::move(o));
  p.tick(0);  // the boot boundary: apply everything, process nothing yet
  return p;
}

int cmd_replay_tape() {
  auto p = replayed_plan(read_stdin(), 128);
  nlohmann::ordered_json nodes, defaults;
  nlohmann::json edges = nlohmann::json::array();
  for (const auto& r : p.nodes()) nodes[r.id] = {{"type", r.type}};
  for (const auto& [f, t] : p.edges()) edges.push_back({{"from", f}, {"to", t}});
  for (const auto& [route, v] : p.defaults()) {
    char* end = nullptr;
    double num = std::strtod(v.c_str(), &end);
    bool numeric = end && *end == 0 && end != v.c_str() &&
                   (v.find('.') != std::string::npos ||
                    v.find_first_not_of("-0123456789") == std::string::npos);
    if (numeric)
      defaults[route] = num;
    else
      defaults[route] = v;
  }
  std::cout << nlohmann::ordered_json{{"nodes", nodes}, {"edges", edges},
                                      {"defaults", defaults}}.dump() << "\n";
  return 0;
}

int cmd_render_tape(double seconds) {
  constexpr int block = syg::movements::hello_cosine_block;
  auto p = replayed_plan(read_stdin(), block);
  int frames = static_cast<int>(seconds * syg::movements::hello_cosine_rate);
  for (int done = 0; done < frames; done += block) {
    int n = frames - done < block ? frames - done : block;
    p.tick(n);
    std::fwrite(p.input_buffer("dac0", "in"), sizeof(float),
                static_cast<std::size_t>(n), stdout);
  }
  return 0;
}

int cmd_roundtrip() {
  auto g = syg::organs::parse_graph(nlohmann::json::parse(read_stdin()));
  std::cout << syg::organs::serialize_graph(g).dump(1) << "\n";
  return 0;
}

int cmd_resolve_hash(const std::string& cid, const std::string& objdir) {
  auto bytes = syg::organs::naive_resolve(objdir, cid);
  std::fwrite(bytes.data(), 1, bytes.size(), stdout);
  return 0;
}

int cmd_swap_audit(const std::string& tape2_path, double seconds) {
  constexpr int block = syg::movements::hello_cosine_block;
  auto p = replayed_plan(read_stdin(), block);
  std::ifstream f2(tape2_path);
  std::ostringstream t2;
  t2 << f2.rdbuf();
  int half = static_cast<int>(seconds * syg::movements::hello_cosine_rate) / 2;
  auto render = [&](int frames) {
    for (int done = 0; done < frames; done += block) {
      int n = frames - done < block ? frames - done : block;
      p.tick(n);
      std::fwrite(p.input_buffer("dac0", "in"), sizeof(float),
                  static_cast<std::size_t>(n), stdout);
    }
  };
  render(half);
  syg::organs::slot_swap(p, syg::crown::read_tape(t2.str()));
  render(half);
  return 0;
}

int cmd_regions() {
  auto in = nlohmann::json::parse(read_stdin());
  auto doc = syg::organs::parse_graph(in.at("graph"));
  // edits apply to the doc first — regions recompute on every edit (EXE-2)
  for (const auto& e : in.value("edits", nlohmann::json::array())) {
    const std::string what = e.at("op");
    if (what == "remove_edge")
      doc.edges.erase(doc.edges.begin() + e.at("index").get<long>());
    else if (what == "add_node")
      doc.nodes.emplace_back(e.at("id"), e.at("type"));
    else if (what == "add_edge")
      doc.edges.emplace_back(e.at("from"), e.at("to"));
    else
      throw std::runtime_error("unknown edit op: " + what);
  }
  auto r = syg::executor::infer_regions(doc);
  nlohmann::json maps = nlohmann::json::array();
  for (const auto& m : r.mappings)
    maps.push_back({{"edge", m.edge},
                    {"from", doc.edges[m.edge].first},
                    {"to", doc.edges[m.edge].second},
                    {"mapping", m.mapping}});
  std::cout << nlohmann::json{{"block", r.block},
                              {"frame", r.frame},
                              {"mappings", maps},
                              {"inert", r.inert},
                              {"errors", r.errors}}.dump() << "\n";
  return 0;
}

int cmd_render_graph(double seconds) {
  // the EXECUTOR semantics (regions + latch), unlike the naive render-tape
  syg::executor::exec_plan p(
      syg::organs::parse_graph(nlohmann::json::parse(read_stdin())), 48000, 128);
  int blocks = static_cast<int>(seconds * 48000) / 128;
  for (int i = 0; i < blocks; ++i)
    std::fwrite(p.pump_block(), sizeof(float), 128, stdout);
  return 0;
}

int cmd_exec_audit() {
  auto in = nlohmann::json::parse(read_stdin());
  syg::executor::exec_plan p(syg::organs::parse_graph(in.at("graph")), 48000, 128);
  int blocks = in.value("blocks", 1);
  auto ops = in.value("ops", nlohmann::json::array());
  auto watch = in.value("watch", std::vector<std::string>{});
  nlohmann::json watched;
  syg::abi::reset_counts();
  long rt_before = syg::abi::counts(syg::abi::phase::process).allocs +
                   syg::abi::counts(syg::abi::phase::process).locks;
  std::map<int, std::vector<const nlohmann::json*>> by_block;
  for (const auto& o : ops) by_block[o.at("block")].push_back(&o);
  for (int blk = 0; blk < blocks; ++blk) {
    for (const auto* op : by_block[blk]) {
      const std::string what = op->value("op", "set_param");
      if (what == "undo")
        p.undo();
      else if (what == "bang")
        p.post_event(op->at("route"), op->value("payload", 0.0));
      else if (what == "set_param")
        p.submit({what, op->at("route"), op->at("value"),
                  op->value("author", "")});
      else
        p.submit({what, op->value("a", ""), op->value("b", ""),
                  op->value("author", "")});
    }
    const float* out = p.pump_block();
    (void)out;
    for (const auto& w : watch) {
      // record [first, min, max] of the watched block-input buffer
      auto id = w.substr(0, w.find('/'));
      // reach the buffer through the sink-independent introspection:
      // watch is only supported on block in-ports via a probe render — use
      // the plan's regions to find it; simplest honest probe: re-run isn't
      // possible, so watch dac0/in == out; other routes via value_of
      if (w == "out") {
        watched[w].push_back({out[0], out[127]});
      } else if (id.find('#') != std::string::npos || true) {
        try {
          watched[w].push_back(p.value_of(w));
        } catch (const std::exception&) {
          watched[w].push_back(nullptr);
        }
      }
    }
  }
  long rt_after = syg::abi::counts(syg::abi::phase::process).allocs +
                  syg::abi::counts(syg::abi::phase::process).locks;
  nlohmann::json recomputes, log = nlohmann::json::array();
  for (const auto& [id, t] : p.doc().nodes)
    if (p.recomputes(id) >= 0) recomputes[id] = p.recomputes(id);
  for (const auto& a : p.log())
    log.push_back({{"op", a.op.op}, {"a", a.op.a}, {"b", a.op.b},
                   {"author", a.op.author}, {"inverse_ops", a.inverse.size()}});
  std::cout << nlohmann::json{{"watched", watched},
                              {"recomputes", recomputes},
                              {"rt_events", rt_after - rt_before},
                              {"process_calls", p.process_calls()},
                              {"log", log},
                              {"serialized", syg::organs::serialize_graph(p.doc())}}
                   .dump() << "\n";
  return 0;
}

nlohmann::json event_graph() {
  return {{"kind", "graph"},
          {"lock", {{"button", "kind:button@v1"}, {"counter", "kind:counter@v1"}}},
          {"topology",
           {{"nodes", {{"button0", {{"type", "button"}}},
                       {"counter0", {{"type", "counter"}}}}},
            {"edges", {{{"from", "button0/out"}, {"to", "counter0/in"}}}}}},
          {"defaults", nlohmann::json::object()}};
}

int cmd_queue_audit(long n_events, int n_threads) {
  // LNG-3.1 / EXE-4.2 / TCF-1: bangs cross a real thread boundary through
  // the queue mapping; nothing drops, duplicates, or (single-producer)
  // reorders. Multi-producer certifies MPSC loss/dup freedom.
  syg::executor::exec_plan p(syg::organs::parse_graph(event_graph()), 48000, 128);
  std::vector<std::thread> producers;
  for (int t = 0; t < n_threads; ++t)
    producers.emplace_back([&, t] {
      long share = n_events / n_threads;
      for (long i = 1; i <= share; ++i)
        p.post_event("button0/out",
                     n_threads == 1 ? static_cast<double>(i) : 0.0);
    });
  long blocks = 0;
  while (std::lround(p.value_of("counter0/out")) < n_events && blocks < 200000) {
    p.pump_block();
    ++blocks;
  }
  for (auto& pr : producers) pr.join();
  for (int i = 0; i < 20; ++i) p.pump_block();  // settle the tail
  std::cout << nlohmann::json{
                   {"count", std::lround(p.value_of("counter0/out"))},
                   {"disorder", std::lround(p.value_of("counter0/errors"))},
                   {"blocks", blocks}}.dump() << "\n";
  return 0;
}

int cmd_swap_storm(int n_ops) {
  // TCF-2: random swaps under load — in-flight ticks complete, values
  // never tear, queued events survive the swaps
  auto g = event_graph();
  auto hello = nlohmann::json::parse(read_stdin());
  for (auto& [id, rec] : hello.at("topology").at("nodes").items())
    g["topology"]["nodes"][id] = rec;
  for (auto& e : hello.at("topology").at("edges")) g["topology"]["edges"].push_back(e);
  for (auto& [k, v] : hello.at("defaults").items()) g["defaults"][k] = v;
  for (auto& [k, v] : hello.at("lock").items()) g["lock"][k] = v;
  syg::executor::exec_plan p(syg::organs::parse_graph(g), 48000, 128);
  std::atomic<bool> done{false};
  std::thread editor([&] {
    unsigned lcg = 12345;
    for (int i = 0; i < n_ops; ++i) {
      lcg = lcg * 1103515245 + 12345;
      switch ((lcg >> 16) % 3) {
        case 0:
          p.submit({"set_param", "osc0/freq",
                    std::to_string(200 + (lcg >> 20) % 400), "storm"});
          break;
        case 1:
          p.submit({"add_node", "n" + std::to_string(i), "noise", "storm"});
          break;
        default:
          p.submit({"remove_node", "n" + std::to_string(i - 1), "", "storm"});
      }
    }
    done = true;
  });
  std::thread poker([&] {
    for (long i = 1; i <= 1000; ++i) p.post_event("button0/out", 0.0);
  });
  long blocks = 0;
  bool finite = true;
  while (!done || std::lround(p.value_of("counter0/out")) < 1000) {
    const float* out = p.pump_block();
    for (int i = 0; i < 128; ++i)
      if (!std::isfinite(out[i])) finite = false;
    if (++blocks > 200000) break;
  }
  editor.join();
  poker.join();
  for (int i = 0; i < 20; ++i) p.pump_block();
  std::cout << nlohmann::json{
                   {"blocks", blocks},
                   {"finite", finite},
                   {"events_counted", std::lround(p.value_of("counter0/out"))},
                   {"applied", p.log().size()},
                   {"rejected", p.rejected_ops()}}.dump() << "\n";
  return 0;
}

int cmd_kinds() {
  std::ifstream f("vocabulary/kinds.json");
  if (!f) throw std::runtime_error("vocabulary/kinds.json not found (run from repo root)");
  auto catalog = nlohmann::json::parse(f);
  // every port promise the linked registry makes must resolve in the catalog
  nlohmann::json unresolved = nlohmann::json::array();
  for (const auto* n : syg::organs::registered_natives()) {
    for (const auto& p : n->in_ports)
      if (!catalog["kinds"].contains(p.kind)) unresolved.push_back(p.kind);
    for (const auto& p : n->out_ports)
      if (!catalog["kinds"].contains(p.kind)) unresolved.push_back(p.kind);
  }
  std::cout << nlohmann::json{{"kinds", catalog["kinds"]},
                              {"unresolved", unresolved}}.dump() << "\n";
  return 0;
}

int cmd_widget_of(const std::string& kind, bool with_range) {
  std::ifstream f("vocabulary/kinds.json");
  auto catalog = nlohmann::json::parse(f);
  const auto& w = catalog.at("kinds").at(kind).at("widget");
  std::string which = with_range && w.contains("with_range")
                          ? w["with_range"].get<std::string>()
                          : w.at("default").get<std::string>();
  std::cout << nlohmann::json{{"widget", which}}.dump() << "\n";
  return 0;
}

int cmd_derive_render(const std::string& objdir, double seconds) {
  // EXE-7: a worker-region pipeline run to completion WITH the recorder on
  // (ADR-031): output committed, recipe provenance written, memo keyed by
  // the recipe (exact class: same inputs, same bytes)
  namespace fs = std::filesystem;
  fs::create_directories(objdir);
  auto graph = nlohmann::json::parse(read_stdin());
  auto graph_bytes = syg::formats::encode_projection(graph);
  auto graph_cid = syg::formats::cid_to_text(syg::formats::cid_of(
      syg::formats::pins::multicodec_dag_cbor, graph_bytes));
  nlohmann::json recipe{{"op", "render"},
                        {"inputs", {{"graph", {{"/", graph_cid}}}}},
                        {"params", {{"seconds", seconds}, {"rate", 48000}}},
                        {"determinism", "exact"}};
  auto recipe_cid = syg::formats::cid_to_text(syg::formats::cid_of(
      syg::formats::pins::multicodec_dag_cbor,
      syg::formats::encode_projection(recipe)));
  auto memo_path = fs::path(objdir) / ("memo-" + recipe_cid);
  if (fs::exists(memo_path)) {
    std::ifstream m(memo_path);
    nlohmann::json hit = nlohmann::json::parse(m);
    hit["memo"] = true;
    hit["passes_run"] = 0;
    std::cout << hit.dump() << "\n";
    return 0;
  }
  // derive: render the graph (the one pass), commit output + provenance
  syg::executor::exec_plan p(syg::organs::parse_graph(graph), 48000, 128);
  syg::formats::byte_vec take;
  int blocks = static_cast<int>(seconds * 48000) / 128;
  for (int i = 0; i < blocks; ++i) {
    const float* out = p.pump_block();
    auto* raw = reinterpret_cast<const std::uint8_t*>(out);
    take.insert(take.end(), raw, raw + 128 * sizeof(float));
  }
  auto put = [&](const syg::formats::byte_vec& cid,
                 const syg::formats::byte_vec& obj) {
    std::ofstream f(fs::path(objdir) / syg::formats::cid_to_text(cid),
                    std::ios::binary);
    f.write(reinterpret_cast<const char*>(obj.data()),
            static_cast<std::streamsize>(obj.size()));
  };
  auto out_cid = syg::formats::cid_to_text(syg::formats::chunk_put(take, put));
  recipe["output"] = {{"/", out_cid}};
  auto prov_bytes = syg::formats::encode_projection(recipe);
  auto prov_cid_b = syg::formats::cid_of(
      syg::formats::pins::multicodec_dag_cbor, prov_bytes);
  put(prov_cid_b, prov_bytes);
  nlohmann::json result{{"memo", false},
                        {"passes_run", 1},
                        {"output", out_cid},
                        {"provenance", syg::formats::cid_to_text(prov_cid_b)},
                        {"recipe", recipe_cid}};
  std::ofstream(memo_path) << nlohmann::json(
      {{"output", out_cid},
       {"provenance", syg::formats::cid_to_text(prov_cid_b)},
       {"recipe", recipe_cid}}).dump();
  std::cout << result.dump() << "\n";
  return 0;
}

int cmd_pins() {
  namespace p = syg::formats::pins;
  nlohmann::ordered_json out;
  out["multicodec"] = {{"dag-cbor", p::multicodec_dag_cbor},
                       {"raw", p::multicodec_raw}};
  out["multihash"] = {{"blake3-256", p::multihash_blake3_256}};
  out["hash_bytes"] = p::hash_bytes;
  out["multibase"] = std::string(1, p::multibase_prefix);
  out["chunk_bytes"] = p::chunk_bytes;
  out["escape"] = p::escape_set;
  out["tape_records"] = p::tape_records;
  out["edit_ops"] = p::edit_ops;
  nlohmann::ordered_json wire;
  for (std::size_t i = 0; i < p::wire_kinds.size(); ++i)
    wire[std::string(p::wire_kinds[i])] = i + 1;
  out["wire_kinds"] = wire;
  std::cout << out.dump() << "\n";
  return 0;
}

int cmd_parse_address() {
  auto a = syg::formats::parse_address(read_stdin());
  static const char* names[] = {"refname", "cid", "peerkey"};
  nlohmann::ordered_json out{{"kind", names[static_cast<int>(a.kind)]},
                             {"head", a.head},
                             {"route", a.route}};
  std::cout << out.dump() << "\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  std::string cmd = argc > 1 ? argv[1] : "";
  try {
    if (cmd == "encode") return cmd_encode();
    if (cmd == "parse-address") return cmd_parse_address();
    if (cmd == "pins") return cmd_pins();
    if (cmd == "hash") return cmd_hash();
    if (cmd == "verify" && argc > 2) return cmd_verify(argv[2]);
    if (cmd == "chunk-put") return cmd_chunk_put();
    if (cmd == "connection-legal") return cmd_connection_legal();
    if (cmd == "render-movement" && argc > 3)
      return cmd_render_movement(argv[2], std::stod(argv[3]));
    if (cmd == "tick-audit") return cmd_tick_audit();
    if (cmd == "codec-selftest" && argc > 2) return cmd_codec_selftest(argv[2]);
    if (cmd == "hook-audit") return syg::harness::hook_audit();
    if (cmd == "create-audit" && argc > 2) return syg::harness::create_audit(argv[2]);
    if (cmd == "fault-audit") return syg::harness::fault_audit();
    if (cmd == "quarantine-audit") return syg::harness::quarantine_audit();
    if (cmd == "replay-tape") return cmd_replay_tape();
    if (cmd == "render-tape" && argc > 2) return cmd_render_tape(std::stod(argv[2]));
    if (cmd == "roundtrip") return cmd_roundtrip();
    if (cmd == "palette") {
      std::cout << syg::organs::palette().dump() << "\n";
      return 0;
    }
    if (cmd == "resolve-hash" && argc > 3) return cmd_resolve_hash(argv[2], argv[3]);
    if (cmd == "swap-audit" && argc > 3) return cmd_swap_audit(argv[2], std::stod(argv[3]));
    if (cmd == "boot-audit" && argc > 2) return syg::harness::boot_audit(argv[2]);
    if (cmd == "unfreeze-stage0") return syg::harness::unfreeze_stage0();
    if (cmd == "park-audit") return syg::harness::park_audit();
    if (cmd == "regions") return cmd_regions();
    if (cmd == "kinds") return cmd_kinds();
    if (cmd == "widget-of" && argc > 2)
      return cmd_widget_of(argv[2], argc > 3 && std::string(argv[3]) == "--range");
    if (cmd == "render-graph" && argc > 2) return cmd_render_graph(std::stod(argv[2]));
    if (cmd == "exec-audit") return cmd_exec_audit();
    if (cmd == "queue-audit" && argc > 2)
      return cmd_queue_audit(std::atol(argv[2]), argc > 3 ? std::atoi(argv[3]) : 1);
    if (cmd == "swap-storm" && argc > 2) return cmd_swap_storm(std::atoi(argv[2]));
    if (cmd == "derive-render" && argc > 3)
      return cmd_derive_render(argv[2], std::stod(argv[3]));
    if (cmd == "naming") {
      std::cout << syg::harness::naming_session(nlohmann::json::parse(read_stdin())).dump() << "\n";
      return 0;
    }
  } catch (const std::exception& e) {
    std::cerr << "syg " << cmd << ": " << e.what() << "\n";
    return 1;
  }
  std::cerr << "syg: unknown subcommand '" << cmd << "' (see conformance/HARNESS.md)\n";
  return 2;
}
