// clause: machinery — the engine pipeline's pass NODE TYPES (CMP-9): the
// compile machinery lives INSIDE these hooks and nowhere else. The engine
// graph instantiates them from the registry and ticks them like any graph;
// compilation is a derivation-mode run of that plan.
#include <cstring>
#include <fstream>
#include <set>

#include <nlohmann/json.hpp>

#include "crown.hpp"
#include "lift.hpp"
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

// recognize-region: expansion + lifting; rules ride the carried doc
void recognize_tick(void*, const crown::svalue* ins, crown::svalue* outs) {
  if (!ins[0].value) return;
  auto doc = organs::expand_subgraphs(
      lift_expand(gen::as_graph(ins[0]), graphs_dir), graphs_dir);
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
// for the block region, so a spliced rule pass changes placement (CMP-9.2)
void choose_tick(void*, const crown::svalue* ins, crown::svalue* outs) {
  if (!ins[0].value) return;
  auto doc = gen::as_graph(ins[0]);
  if (ins[1].value && !gen::as_text(ins[1]).empty()) {
    std::string rules = doc.defaults.value("__rules", "");
    doc.defaults["__rules"] =
        rules.empty() ? gen::as_text(ins[1]) : rules + ";" + gen::as_text(ins[1]);
  }
  auto regions = infer_regions(doc);
  if (!regions.errors.empty())
    throw std::runtime_error("compile: " + regions.errors.front());
  std::set<std::string> block(regions.block.begin(), regions.block.end());
  std::set<std::string> frame(regions.frame.begin(), regions.frame.end());
  const std::string rules = doc.defaults.value("__rules", "");
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
  doc.defaults["__regions"] = {{"block", block}, {"frame", frame}};
  doc.defaults["__mappings"] = maps;
  outs[0] = gen::make_graph(std::move(doc));
}

// realize (interpret backend): assemble the execution graph + the map
void realize_tick(void*, const crown::svalue* ins, crown::svalue* outs) {
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
                        std::vector<crown::port_decl> outs) {
  return {name,    create,  destroy, set_num, set_text, no_process,
          nullptr, std::move(ins), std::move(outs), false, false,
          nullptr, tick,    nullptr, nullptr, false,   nullptr};
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
    "choose-adapters", null_create, null_destroy, no_num, no_text,
    choose_tick, {{"in", "graph", "value"}, {"rules", "text", "value"}},
    {{"out", "graph", "value"}});
const crown::native_type realize_native = pass(
    "realize", null_create, null_destroy, no_num, no_text, realize_tick,
    {{"in", "graph", "value"}, {"backend", "text", "value"}},
    {{"out", "text", "value"}});

}  // namespace syg::executor
