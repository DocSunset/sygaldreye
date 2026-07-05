// clause: machinery — the subgraph organ
#include "subgraph/subgraph.hpp"

#include "phase.hpp"

#include <set>
#include <stdexcept>

#include "registry_face/registry_face.hpp"

namespace syg::organs {
namespace {

const crown::native_type* native_named(const std::string& n) {
  for (const auto* t : registered_natives())
    if (n == t->name) return t;
  return nullptr;
}

graph_doc expand(graph_doc doc, const template_loader& load,
                 std::set<std::string>& stack) {
  graph_doc out;
  out.kind = doc.kind;
  out.lock = doc.lock;
  out.lift_key = doc.lift_key;
  out.inlets = doc.inlets;
  out.outlets = doc.outlets;
  out.defaults = nlohmann::json::object();
  std::map<std::string, graph_doc> spawned;  // subgraph id -> expanded inner
  for (const auto& [id, type] : doc.nodes) {
    if (native_named(type)) {
      out.nodes.emplace_back(id, type);
      continue;
    }
    auto tj = load(type);
    if (!tj) throw std::runtime_error("no such node type: " + type);
    if (!stack.insert(type).second)
      throw std::runtime_error("template cycle through " + type);
    auto inner = expand(parse_graph(*tj), load, stack);
    stack.erase(type);
    for (const auto& [iid, itype] : inner.nodes)
      out.nodes.emplace_back(id + "." + iid, itype);
    for (const auto& [f, t] : inner.edges)
      out.edges.emplace_back(id + "." + f, id + "." + t);
    for (const auto& [route, v] : inner.defaults.items())
      out.defaults[id + "." + route] = v;
    spawned[id] = std::move(inner);
  }
  auto rewire = [&](const std::string& route, bool inlet) -> std::string {
    auto slash = route.find('/');
    auto id = route.substr(0, slash);
    auto it = spawned.find(id);
    if (it == spawned.end()) return route;
    const auto& map = inlet ? it->second.inlets : it->second.outlets;
    auto port = route.substr(slash + 1);
    if (!map.contains(port))
      throw std::runtime_error("subgraph " + id + " has no port " + port);
    return id + "." + map[port].get<std::string>();
  };
  for (const auto& [f, t] : doc.edges)
    out.edges.emplace_back(rewire(f, false), rewire(t, true));
  for (const auto& [route, v] : doc.defaults.items()) {
    auto slash = route.find('/');
    if (spawned.count(route.substr(0, slash))) {
      // creation-args: the node entry's param is the inner inlet's default
      out.defaults[rewire(route, true)] = v;
    } else {
      out.defaults[route] = v;
    }
  }
  return out;
}

}  // namespace

graph_doc expand_subgraphs(graph_doc doc, const template_loader& load) {
  abi::note_compile_work();
  std::set<std::string> stack;
  return expand(std::move(doc), load, stack);
}

std::string holder_within(const nlohmann::json& template_json,
                          const template_loader& load) {
  auto doc = parse_graph(template_json);
  for (const auto& [id, type] : doc.nodes) {
    if (const auto* t = native_named(type)) {
      if (t->resource_holder) return id + " (" + type + ")";
    } else if (auto tj = load(type)) {
      auto inner = holder_within(*tj, load);
      if (!inner.empty()) return id + "." + inner;
    }
  }
  return "";
}

}  // namespace syg::organs
