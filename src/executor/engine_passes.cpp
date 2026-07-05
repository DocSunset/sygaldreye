// clause: machinery — the engine pipeline's pass NODE TYPES (CMP-9): the
// compile machinery lives INSIDE these hooks and nowhere else. The engine
// graph instantiates them from the registry and ticks them like any graph;
// compilation is a derivation-mode run of that plan.
#include <cstring>
#include <fstream>
#include <set>

#include <nlohmann/json.hpp>

#include "codegen.hpp"
#include "crown.hpp"
#include "lift.hpp"
#include "phase.hpp"
#include "registry_face/registry_face.hpp"
#include "store.hpp"
#include "parser/parser.hpp"
#include "regions.hpp"
#include "subgraph/subgraph.hpp"
#include "svalue_accessors.hpp"

namespace syg::executor {
namespace {

namespace gen = syg::generated;

std::optional<nlohmann::json> graphs_dir(const std::string& type) {
  std::ifstream f("graphs/" + type + ".json");
  if (!f) return std::nullopt;
  return nlohmann::json::parse(f);
}

// receive: the app graph enters the engine here (a param at this rung; a
// mesh OPS/PLACE surface later)
struct receive_state {
  crown::svalue held;
};
void receive_set_text(void* s, const char* port, const char* v) {
  if (std::strcmp(port, "app")) return;
  static_cast<receive_state*>(s)->held =
      gen::make_graph(organs::parse_graph(nlohmann::json::parse(v)));
}
void receive_tick(void* s, const crown::svalue*, crown::svalue* outs) {
  outs[0] = static_cast<receive_state*>(s)->held;
}

// fanin: rules gather in port order — ORDER IS WIRING, visible as slots
void fanin_tick(void*, const crown::svalue* ins, crown::svalue* outs) {
  std::string joined;
  for (int i = 0; i < 4; ++i)
    if (ins[i].value && !gen::as_text(ins[i]).empty()) {
      if (!joined.empty()) joined += ",";
      joined += gen::as_text(ins[i]);
    }
  outs[0] = gen::make_text(std::move(joined));
}

// recognize-region: expansion + lifting; rules ride the carried doc.
// Nothing-to-expand is identity BY CONSTRUCTION (no structural work); a
// real expansion counts on the structural ledger (CMP-1.2's witness)
bool can_expand(const organs::graph_doc& doc) {
  for (const auto& [id, t] : doc.nodes) {
    const crown::native_type* n = nullptr;
    for (const auto* c : organs::registered_natives())
      if (t == c->name) n = c;
    if (!n) return true;  // a graph-authored type: subgraph expansion
    for (const auto& p : n->out_ports)
      if (std::string_view(p.kind) == "span") return true;  // lift site
  }
  return false;
}
void recognize_tick(void*, const crown::svalue* ins, crown::svalue* outs) {
  if (!ins[0].value) return;
  auto doc = gen::as_graph(ins[0]);
  if (can_expand(doc)) {
    abi::note_structural_run();
    doc = organs::expand_subgraphs(lift_expand(std::move(doc), graphs_dir),
                                   graphs_dir);
  }
  doc.defaults["__rules"] = ins[1].value ? gen::as_text(ins[1]) : "";
  outs[0] = gen::make_graph(std::move(doc));
}

// construct-context: rate/block join the carried doc
struct context_state {
  double rate = 48000, block = 128;
};
void context_set_num(void* s, const char* port, double v) {
  auto* st = static_cast<context_state*>(s);
  if (!std::strcmp(port, "rate")) st->rate = v;
  if (!std::strcmp(port, "block")) st->block = v;
}
void context_tick(void* s, const crown::svalue* ins, crown::svalue* outs) {
  if (!ins[0].value) return;
  auto* st = static_cast<context_state*>(s);
  auto doc = gen::as_graph(ins[0]);
  doc.defaults["__context"] = {{"rate", st->rate}, {"block", st->block}};
  outs[0] = gen::make_graph(std::move(doc));
}

// choose-adapters: regions + boundary mappings join the carried doc; the
// rules lane has CONSEQUENCE here — a `block:<id>` rule claims an instance
// for the block region, so a spliced rule pass changes placement (CMP-9.2).
// The structural derivation is a committed, memoized store derivation
// keyed on topology + rules (CMP-1.2): a defaults-only edit answers from
// the store; only a real inference counts on the structural ledger.
struct choose_state {
  store::peer_store* store = nullptr;
};
void choose_set_context(void* s, const void* ctx) {
  static_cast<choose_state*>(s)->store =
      static_cast<store::peer_store*>(const_cast<void*>(
          static_cast<const crown::node_context*>(ctx)->store));
}
nlohmann::json choose_structure(const organs::graph_doc& doc,
                                const std::string& rules) {
  abi::note_structural_run();
  auto regions = infer_regions(doc);
  if (!regions.errors.empty())
    throw std::runtime_error("compile: " + regions.errors.front());
  std::set<std::string> block(regions.block.begin(), regions.block.end());
  std::set<std::string> frame(regions.frame.begin(), regions.frame.end());
  for (std::size_t i = 0; i < rules.size();) {
    auto j = rules.find_first_of(",;", i);
    if (j == std::string::npos) j = rules.size();
    auto tok = rules.substr(i, j - i);
    if (tok.rfind("block:", 0) == 0 && frame.erase(tok.substr(6)))
      block.insert(tok.substr(6));
    i = j + 1;
  }
  nlohmann::json maps = nlohmann::json::array();
  for (const auto& m : regions.mappings)
    maps.push_back({{"edge", m.edge}, {"mapping", m.mapping}});
  return {{"block", block}, {"frame", frame}, {"mappings", maps}};
}
void choose_tick(void* s, const crown::svalue* ins, crown::svalue* outs) {
  if (!ins[0].value) return;
  auto doc = gen::as_graph(ins[0]);
  if (ins[1].value && !gen::as_text(ins[1]).empty()) {
    std::string rules = doc.defaults.value("__rules", "");
    doc.defaults["__rules"] =
        rules.empty() ? gen::as_text(ins[1]) : rules + ";" + gen::as_text(ins[1]);
  }
  const std::string rules = doc.defaults.value("__rules", "");
  auto* store = static_cast<choose_state*>(s)->store;
  nlohmann::json structure, recipe;
  if (store) {
    nlohmann::ordered_json nodes;
    for (const auto& [id, t] : doc.nodes) nodes[id] = t;
    auto edges = nlohmann::json::array();
    for (const auto& [f, t] : doc.edges)
      edges.push_back({{"from", f}, {"to", t}});
    recipe = {{"op", "compile-structure"},
              {"inputs",
               {{"topology",
                 {{"/", store->put_node({{"nodes", nodes}, {"edges", edges}},
                                        false)}}}}},
              {"rules", rules},
              {"determinism", "exact"}};
    if (auto hit = store->memo_lookup(recipe))
      structure = store->get_node(hit->output);
  }
  if (structure.is_null()) {
    structure = choose_structure(doc, rules);
    if (store) store->commit_derivation(recipe, structure);
  }
  doc.defaults["__regions"] = {{"block", structure["block"]},
                               {"frame", structure["frame"]}};
  doc.defaults["__mappings"] = structure["mappings"];
  outs[0] = gen::make_graph(std::move(doc));
}

// realize: assemble the execution graph + the map. Two backends
// (ADR-014): interpret (instantiate natives + plan) and codegen (the
// freezer — emit a fused C++ movement, committed as a raw artifact whose
// provenance is the compile recipe itself). Same passes, same map.
struct realize_state {
  store::peer_store* store = nullptr;
};
void realize_set_context(void* s, const void* ctx) {
  static_cast<realize_state*>(s)->store =
      static_cast<store::peer_store*>(const_cast<void*>(
          static_cast<const crown::node_context*>(ctx)->store));
}
void realize_tick(void* s, const crown::svalue* ins, crown::svalue* outs) {
  if (!ins[0].value) return;
  const auto& doc = gen::as_graph(ins[0]);
  nlohmann::ordered_json xnodes, map;
  std::set<std::string> in_block;
  for (const auto& b : doc.defaults.value("__regions", nlohmann::ordered_json::object())
                           .value("block", nlohmann::json::array()))
    in_block.insert(b.get<std::string>());
  for (const auto& [id, t] : doc.nodes) {
    if (id.starts_with("__")) continue;
    std::string region = in_block.count(id) ? "block" : "frame";
    xnodes[region + "/" + id] = {{"type", t}};
    auto app_route = "nodes/" + id.substr(0, id.find('#'));
    if (!map.contains(app_route)) map[app_route] = region + "/" + id;
  }
  nlohmann::ordered_json defaults;
  for (const auto& [k, v] : doc.defaults.items())
    if (!k.starts_with("__")) defaults[k] = v;
  nlohmann::ordered_json execution{
      {"kind", "execution-graph"},
      {"nodes", xnodes},
      {"mappings", doc.defaults.value("__mappings", nlohmann::ordered_json::array())},
      {"map", map},
      {"defaults", defaults},
      {"rules", doc.defaults.value("__rules", "")},
      {"context", doc.defaults.value("__context", nlohmann::ordered_json::object())},
      {"backend", ins[1].value && !gen::as_text(ins[1]).empty()
                      ? gen::as_text(ins[1])
                      : "interpret"}};
  if (execution["backend"] == "codegen") {
    std::ifstream lf("vocabulary/codegen.json");
    auto ledger = nlohmann::json::parse(lf).at("natives");
    int block = static_cast<int>(
        doc.defaults.value("__context", nlohmann::ordered_json::object())
            .value("block", 128.0));
    auto e = emit_frozen(doc, ledger, block);
    auto* store = static_cast<realize_state*>(s)->store;
    if (!store)
      throw std::runtime_error("codegen realize needs the store in context");
    formats::byte_vec bytes(e.source.begin(), e.source.end());
    execution["artifact"] = {{"/", store->put_raw(bytes, false)}};
    execution["tier"] = e.tier;
    execution["tier_culprit"] = e.tier_culprit;
  }
  outs[0] = gen::make_text(execution.dump());
}

void no_process(void*, const float* const*, float* const*, int) noexcept {}

crown::native_type pass(const char* name, void* (*create)(),
                        void (*destroy)(void*),
                        void (*set_num)(void*, const char*, double),
                        void (*set_text)(void*, const char*, const char*),
                        void (*tick)(void*, const crown::svalue*,
                                     crown::svalue*),
                        std::vector<crown::port_decl> ins,
                        std::vector<crown::port_decl> outs,
                        void (*set_context)(void*, const void*) = nullptr) {
  return {name,    create,  destroy, set_num, set_text, no_process,
          nullptr, std::move(ins), std::move(outs), false, false,
          nullptr, tick,    nullptr, nullptr, false,   set_context};
}
void* null_create() { return nullptr; }
void null_destroy(void*) {}
void no_num(void*, const char*, double) {}
void no_text(void*, const char*, const char*) {}

}  // namespace

extern const crown::native_type receive_native, fanin_native,
    recognize_region_native, construct_context_native,
    choose_adapters_native, realize_native;

const crown::native_type receive_native = pass(
    "receive", [] { return static_cast<void*>(new receive_state()); },
    [](void* s) { delete static_cast<receive_state*>(s); }, no_num,
    receive_set_text, receive_tick, {{"app", "text", "value"}},
    {{"out", "graph", "value"}});
const crown::native_type fanin_native = pass(
    "fanin", null_create, null_destroy, no_num, no_text, fanin_tick,
    {{"in0", "text", "value"}, {"in1", "text", "value"},
     {"in2", "text", "value"}, {"in3", "text", "value"}},
    {{"out", "text", "value"}});
const crown::native_type recognize_region_native = pass(
    "recognize-region", null_create, null_destroy, no_num, no_text,
    recognize_tick, {{"in", "graph", "value"}, {"rules", "text", "value"}},
    {{"out", "graph", "value"}});
const crown::native_type construct_context_native = pass(
    "construct-context",
    [] { return static_cast<void*>(new context_state()); },
    [](void* s) { delete static_cast<context_state*>(s); }, context_set_num,
    no_text, context_tick, {{"in", "graph", "value"}},
    {{"out", "graph", "value"}});
const crown::native_type choose_adapters_native = pass(
    "choose-adapters",
    [] { return static_cast<void*>(new choose_state()); },
    [](void* s) { delete static_cast<choose_state*>(s); }, no_num, no_text,
    choose_tick, {{"in", "graph", "value"}, {"rules", "text", "value"}},
    {{"out", "graph", "value"}}, choose_set_context);
const crown::native_type realize_native = pass(
    "realize",
    [] { return static_cast<void*>(new realize_state()); },
    [](void* s) { delete static_cast<realize_state*>(s); }, no_num, no_text,
    realize_tick, {{"in", "graph", "value"}, {"backend", "text", "value"}},
    {{"out", "text", "value"}}, realize_set_context);

}  // namespace syg::executor
