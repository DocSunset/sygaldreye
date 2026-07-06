#include "walk_session.hpp"

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "parser/parser.hpp"
#include "registry_face/registry_face.hpp"
#include "store.hpp"

namespace syg::harness {
namespace {

using nlohmann::json;
using store::peer_store;

bool is_link(const json& v) {
  return v.is_object() && v.size() == 1 && v.contains("/") && v["/"].is_string();
}

// The node-type declarations, committed once so the walk can reach them
// (ch. 2 / rung 4): a node's type is a name into the graph's lock -> the
// committed type node -> its ports.
std::map<std::string, std::string> commit_types(peer_store& s) {
  std::map<std::string, std::string> cids;
  for (const auto* n : organs::all_natives()) {
    json ports;
    for (const auto& p : n->in_ports)
      ports[p.name] = {{"dir", "in"}, {"kind", p.kind}, {"discipline", p.discipline}};
    for (const auto& p : n->out_ports)
      ports[p.name] = {{"dir", "out"}, {"kind", p.kind}, {"discipline", p.discipline}};
    cids[n->name] = s.put_node(
        {{"kind", "node-type"}, {"name", n->name}, {"ports", ports}}, false);
  }
  return cids;
}

// A resolved position: the value under `here`, plus the enclosing graph's lock
// (so a node's type name can hop to its committed type node).
struct position {
  json value;
  json lock = json::object();
};

// One step from `here`. Deref links; a node's type name hops through the lock.
position step(const peer_store& s, const position& cur, const std::string& name) {
  const json& v = cur.value;
  if (v.is_object() && v.contains(name)) {
    json next = v.at(name);
    json lock = v.contains("lock") ? v.at("lock") : cur.lock;
    if (is_link(next)) {
      auto obj = s.get_node(next["/"].get<std::string>());
      json l = obj.contains("lock") ? obj["lock"] : lock;
      return {obj, l};
    }
    return {next, lock};
  }
  // the type hop: `here` is a type NAME the lock knows -> its type node
  if (v.is_string() && v.get<std::string>() == name && cur.lock.contains(name)) {
    auto cid = cur.lock.at(name).at("/").get<std::string>();
    return {s.get_node(cid), cur.lock};
  }
  throw std::runtime_error("no such step: " + name);
}

// The frontier: the outward steps reachable from `here`, labeled by kind.
// Paginated + demand-driven so a node with 100k links never materializes all
// (EDR-5.2). Returns {total, page, page_size, steps:[{name,kind}]}.
json frontier(const position& cur, std::size_t page, std::size_t page_size) {
  const json& v = cur.value;
  const std::size_t lo = page * page_size, hi = lo + page_size;
  json steps = json::array();
  std::size_t total = 0;
  auto emit = [&](const std::string& name, const json& child) {
    // stream: only the requested window ever becomes a result object, so a
    // node with 100k links never materializes all of them (EDR-5.2).
    if (total >= lo && total < hi) {
      std::string kind = is_link(child) ? "link"
                         : child.is_object() ? "map"
                         : child.is_array() ? "list"
                         : child.is_null() ? "type"
                         : "leaf";
      steps.push_back({{"name", name}, {"kind", kind}});
    }
    ++total;
  };
  if (v.is_object()) {
    for (auto it = v.begin(); it != v.end(); ++it)
      if (it.key() != "lock") emit(it.key(), it.value());  // lock is machinery
  } else if (v.is_string() && cur.lock.contains(v.get<std::string>())) {
    emit(v.get<std::string>(), json(nullptr));  // the type hop: one step out
  }
  return {{"total", total}, {"page", page}, {"page_size", page_size},
          {"steps", steps}};
}

}  // namespace

int walk_session(const nlohmann::json& in) {
  peer_store s("walker");
  // the ground: a synthetic node whose frontier is every named ref.
  json ground = json::object();

  const json cfg = in.value("store", json::object());
  if (cfg.value("types", false)) commit_types(s);
  const json cfg_graphs = cfg.value("graphs", json::object());
  const json cfg_fanout = cfg.value("fanout", json::object());
  for (auto& [ref, graph] : cfg_graphs.items()) {
    // commit the graph with a lock over its node types (the committed chain)
    auto doc = organs::parse_graph(graph);
    auto tcids = commit_types(s);
    json lock = json::object();
    for (const auto& [id, tname] : doc.nodes)
      if (tcids.count(tname)) lock[tname] = {{"/", tcids.at(tname)}};
    doc.lock = lock;
    auto cid = s.put_node(organs::serialize_graph(doc), false);
    s.bind_ref(ref, cid);
    ground[ref] = {{"/", cid}};
  }
  // a wide fanout node for the pagination criterion (EDR-5.2)
  for (auto& [ref, count] : cfg_fanout.items()) {
    json obj = json::object();
    long n = count.get<long>();
    // links point back at the ground graphs (real links, content-addressed)
    std::string tgt = ground.begin() != ground.end()
                          ? ground.begin().value()["/"].get<std::string>()
                          : s.put_node({{"kind", "leaf"}}, false);
    for (long i = 0; i < n; ++i) obj["k" + std::to_string(i)] = {{"/", tgt}};
    auto cid = s.put_node(obj, false);
    s.bind_ref(ref, cid);
    ground[ref] = {{"/", cid}};
  }

  auto resolve = [&](const json& path) {
    position cur{ground, json::object()};
    for (const auto& stepname : path) cur = step(s, cur, stepname.get<std::string>());
    return cur;
  };

  json results = json::array();
  for (const auto& op : in.value("ops", json::array())) {
    const std::string what = op.at("op");
    json r;
    if (what == "ground") {
      r = {{"here", "ground"},
           {"frontier", frontier({ground, {}}, 0, op.value("page_size", 64))}};
    } else if (what == "walk") {
      auto path = op.at("path");
      auto cur = resolve(path);
      r = {{"path", path}, {"here", path.empty() ? json("ground") : path.back()},
           {"frontier", frontier(cur, 0, op.value("page_size", 64))}};
    } else if (what == "frontier") {
      auto cur = resolve(op.value("path", json::array()));
      r = frontier(cur, op.value("page", 0), op.value("page_size", 64));
    } else if (what == "value") {
      r = {{"value", resolve(op.value("path", json::array())).value}};
    } else if (what == "mark") {
      // the marked subgraph + hand-authored links = a durable dataset.
      json marks = json::array();
      for (const auto& p : op.at("paths")) marks.push_back(p);
      json mapdata{{"kind", "marked-map"}, {"marks", marks},
                   {"links", op.value("links", json::array())}};
      auto cid = s.put_node(mapdata, true);
      if (op.contains("as")) s.bind_ref(op.at("as"), cid);
      r = {{"cid", cid}};
    } else if (what == "open-mark") {
      std::string cid = op.contains("ref") ? *s.ref(op.at("ref"))
                                           : op.at("cid").get<std::string>();
      r = {{"map", s.get_node(cid)}};
    } else {
      r = {{"error", "unknown op"}, {"op", what}};
    }
    results.push_back(r);
  }
  std::cout << json{{"results", results}}.dump() << "\n";
  return 0;
}

}  // namespace syg::harness
