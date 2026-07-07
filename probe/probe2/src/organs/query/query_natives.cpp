// clause: machinery — the query four as REALIZED node types (ADR-024/034):
// traverse · filter · join · fixpoint (+ seed), exchanging cidset values
// over the structured lane, the store arriving by context injection
#include <cstring>
#include <set>

#include <nlohmann/json.hpp>

#include "crown.hpp"
#include "query/query.hpp"
#include "store.hpp"
#include "svalue_accessors.hpp"

namespace syg::organs {
namespace {

namespace gen = syg::generated;
using cidvec = std::vector<std::string>;

struct q_state {
  const store::peer_store* store = nullptr;
  std::string via = "backlinks", key, equals, mode = "and";
  cidvec cids;
  bool all = false;
};
void q_set_context(void* s, const void* ctx) {
  static_cast<q_state*>(s)->store = static_cast<const store::peer_store*>(
      static_cast<const syg::crown::node_context*>(ctx)->store);
}
void q_set_num(void* s, const char* port, double v) {
  if (!std::strcmp(port, "all")) static_cast<q_state*>(s)->all = v != 0.0;
}
void q_set_text(void* s, const char* port, const char* v) {
  auto* st = static_cast<q_state*>(s);
  if (!std::strcmp(port, "via")) st->via = v;
  if (!std::strcmp(port, "key")) st->key = v;
  if (!std::strcmp(port, "equals")) st->equals = v;
  if (!std::strcmp(port, "mode")) st->mode = v;
  if (!std::strcmp(port, "cids")) {
    st->cids.clear();
    for (const auto& c : nlohmann::json::parse(v)) st->cids.push_back(c);
  }
}
cidvec in_set(const syg::crown::svalue& v) {
  return v.value ? gen::as_cidset(v) : cidvec{};
}

void seed_tick(void* s, const syg::crown::svalue*, syg::crown::svalue* outs) {
  auto* st = static_cast<q_state*>(s);
  cidvec out = st->cids;
  if (st->all && st->store)
    for (const auto& [cid, b] : st->store->objects()) out.push_back(cid);
  outs[0] = gen::make_cidset(std::move(out));
}

void traverse_tick(void* s, const syg::crown::svalue* ins,
                   syg::crown::svalue* outs) {
  auto* st = static_cast<q_state*>(s);
  if (!st->store) return;
  auto in = in_set(ins[0]);
  auto next = query_step(*st->store, {in.begin(), in.end()}, st->via);
  outs[0] = gen::make_cidset({next.begin(), next.end()});
}

void filter_tick(void* s, const syg::crown::svalue* ins,
                 syg::crown::svalue* outs) {
  auto* st = static_cast<q_state*>(s);
  cidvec out;
  if (st->store)
    for (const auto& c : in_set(ins[0])) {
      try {
        auto node = st->store->get_node(c);
        if (node.is_object() && node.contains(st->key) &&
            node[st->key] == st->equals)
          out.push_back(c);
      } catch (const std::exception&) {
      }
    }
  outs[0] = gen::make_cidset(std::move(out));
}

void join_tick(void* s, const syg::crown::svalue* ins,
               syg::crown::svalue* outs) {
  auto* st = static_cast<q_state*>(s);
  auto a = in_set(ins[0]);
  auto b = in_set(ins[1]);
  std::set<std::string> bs(b.begin(), b.end()), out;
  if (st->mode == "or") {
    out.insert(a.begin(), a.end());
    out.insert(b.begin(), b.end());
  } else {
    for (const auto& c : a)
      if (bs.count(c)) out.insert(c);
  }
  outs[0] = gen::make_cidset({out.begin(), out.end()});
}

void fixpoint_tick(void* s, const syg::crown::svalue* ins,
                   syg::crown::svalue* outs) {
  auto* st = static_cast<q_state*>(s);
  if (!st->store) return;
  auto start = in_set(ins[0]);
  // visited-set semantics: terminates by construction (LNG-10.3)
  std::set<std::string> visited(start.begin(), start.end());
  std::set<std::string> frontier = visited;
  while (!frontier.empty()) {
    auto next = query_step(*st->store, frontier, st->via);
    auto outs2 = query_outputs_of(*st->store, next);
    next.insert(outs2.begin(), outs2.end());
    frontier.clear();
    for (const auto& c : next)
      if (visited.insert(c).second) frontier.insert(c);
  }
  outs[0] = gen::make_cidset({visited.begin(), visited.end()});
}

void no_process(void*, const float* const*, float* const*, int) noexcept {}

syg::crown::native_type make(const char* name,
                             void (*tick)(void*, const syg::crown::svalue*,
                                          syg::crown::svalue*),
                             std::vector<syg::crown::port_decl> ins) {
  return {name, [] { return static_cast<void*>(new q_state()); },
          [](void* s) { delete static_cast<q_state*>(s); },
          q_set_num, q_set_text, no_process, nullptr, std::move(ins),
          {{"out", "cidset", "value"}}, false, false, nullptr, tick, nullptr,
          nullptr, false, q_set_context};
}

}  // namespace

extern const syg::crown::native_type seed_native, traverse_native,
    filter_native, join_native, fixpoint_native;
const syg::crown::native_type seed_native =
    make("seed", seed_tick, {{"cids", "text", "value"}});
const syg::crown::native_type traverse_native =
    make("traverse", traverse_tick, {{"in", "cidset", "value"}});
const syg::crown::native_type filter_native =
    make("filter", filter_tick, {{"in", "cidset", "value"}});
const syg::crown::native_type join_native =
    make("join", join_tick,
         {{"a", "cidset", "value"}, {"b", "cidset", "value"}});
const syg::crown::native_type fixpoint_native =
    make("fixpoint", fixpoint_tick, {{"in", "cidset", "value"}});

}  // namespace syg::organs
