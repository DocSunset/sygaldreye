// clause: scaffolding (dissolves: LNG-11.4) — a bespoke evaluator over
// query docs; ADR-034 retro-mark: the query four become realized
// instances exchanging set values over the structured lane
#include "query/query.hpp"

#include <map>
#include <stdexcept>

namespace syg::organs {
namespace {

using cidset = std::set<std::string>;

// one traversal step: every cid reachable from `from` by one link
cidset step(const store::peer_store& s, const cidset& from,
            const std::string& via) {
  cidset out;
  for (const auto& c : from) {
    if (via == "backlinks") {
      for (const auto& b : s.backlinks(c)) out.insert(b);
    } else {  // "links": downstream through the object's own link values,
      // ref names resolving through the ref table (cycles possible!)
      try {
        auto node = s.get_node(c);
        std::function<void(const nlohmann::json&)> walk =
            [&](const nlohmann::json& v) {
              if (v.is_object()) {
                if (v.size() == 1 && v.contains("/") && v["/"].is_string()) {
                  out.insert(v["/"].get<std::string>());
                  return;
                }
                for (const auto& [k, x] : v.items()) walk(x);
              } else if (v.is_array()) {
                for (const auto& x : v) walk(x);
              } else if (v.is_string()) {
                if (const auto* r = s.ref(v.get<std::string>()))
                  out.insert(*r);  // a refname link: the conferred-mutable hop
              }
            };
        walk(node);
      } catch (const std::exception&) {
      }
    }
  }
  return out;
}

// the output half of a backlink hop: provenance records pull their outputs
cidset outputs_of(const store::peer_store& s, const cidset& records) {
  cidset out;
  for (const auto& c : records) {
    try {
      auto node = s.get_node(c);
      if (node.is_object() && node.contains("output") &&
          node["output"].is_object() && node["output"].contains("/"))
        out.insert(node["output"]["/"].get<std::string>());
    } catch (const std::exception&) {
    }
  }
  return out;
}

}  // namespace

std::set<std::string> eval_query(const graph_doc& q, const store::peer_store& s,
                                 long* node_evals,
                                 const std::set<std::string>* delta) {
  std::map<std::string, cidset> results;
  auto param = [&](const std::string& id, const std::string& key) {
    auto route = id + "/" + key;
    return q.defaults.contains(route) ? q.defaults[route]
                                      : nlohmann::ordered_json();
  };
  auto input_of = [&](const std::string& id, const std::string& port) {
    for (const auto& [f, t] : q.edges)
      if (t == id + "/" + port) return results.at(f.substr(0, f.find('/')));
    return cidset{};
  };
  long evals = 0;
  // dependency order (query docs arrive with arbitrary key order)
  std::vector<std::pair<std::string, std::string>> order;
  {
    std::set<std::string> done;
    while (order.size() < q.nodes.size()) {
      bool grew = false;
      for (const auto& [id, type] : q.nodes) {
        if (done.count(id)) continue;
        bool ready = true;
        for (const auto& [f, t] : q.edges)
          if (t.substr(0, t.find('/')) == id &&
              !done.count(f.substr(0, f.find('/'))))
            ready = false;
        if (ready) {
          order.emplace_back(id, type);
          done.insert(id);
          grew = true;
        }
      }
      if (!grew) throw std::runtime_error("query graph has a cycle");
    }
  }
  for (const auto& [id, type] : order) {
    ++evals;
    if (type == "seed") {
      cidset set;
      if (delta) {
        set = *delta;  // the standing form: the new commit IS the seed
      } else if (param(id, "all").is_boolean() && param(id, "all")) {
        for (const auto& [cid, b] : s.objects()) set.insert(cid);
      } else {
        for (const auto& c : param(id, "cids")) set.insert(c.get<std::string>());
      }
      results[id] = std::move(set);
    } else if (type == "traverse") {
      results[id] = step(s, input_of(id, "in"), param(id, "via"));
    } else if (type == "filter") {
      cidset out;
      for (const auto& c : input_of(id, "in")) {
        try {
          auto node = s.get_node(c);
          auto key = param(id, "key");
          if (key.is_string() && node.is_object() &&
              node.contains(key.get<std::string>()) &&
              node[key.get<std::string>()] == param(id, "equals"))
            out.insert(c);
        } catch (const std::exception&) {
        }
      }
      results[id] = std::move(out);
    } else if (type == "join") {
      auto a = input_of(id, "a");
      auto b = input_of(id, "b");
      cidset out;
      if (param(id, "mode") == "or") {
        out = a;
        out.insert(b.begin(), b.end());
      } else {
        for (const auto& c : a)
          if (b.count(c)) out.insert(c);
      }
      results[id] = std::move(out);
    } else if (type == "fixpoint") {
      // visited-set semantics: terminates by construction (LNG-10.3)
      cidset visited = input_of(id, "in"), frontier = visited;
      auto via = param(id, "via").is_string()
                     ? param(id, "via").get<std::string>()
                     : "backlinks";
      while (!frontier.empty()) {
        ++evals;
        auto next = step(s, frontier, via);
        auto outs = outputs_of(s, next);
        next.insert(outs.begin(), outs.end());
        frontier.clear();
        for (const auto& c : next)
          if (visited.insert(c).second) frontier.insert(c);
      }
      results[id] = std::move(visited);
    } else {
      throw std::runtime_error("not a query primitive: " + type);
    }
  }
  if (node_evals) *node_evals = evals;
  if (order.empty()) return {};
  // the result is the sink: the node nothing consumes
  for (const auto& [id, type] : order) {
    bool consumed = false;
    for (const auto& [f, t] : q.edges)
      if (f.substr(0, f.find('/')) == id) consumed = true;
    if (!consumed) return results.at(id);
  }
  return results.at(order.back().first);
}

}  // namespace syg::organs
