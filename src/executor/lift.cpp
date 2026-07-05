// clause: machinery — executor package (plans, pacing, mappings)
#include "lift.hpp"

#include <set>
#include <stdexcept>

#include "registry_face/registry_face.hpp"

namespace syg::executor {
namespace {

const crown::native_type* native_named(const std::string& n) {
  for (const auto* t : organs::registered_natives())
    if (n == t->name) return t;
  return nullptr;
}

// the kind a consumer port promises, through templates' inlets if needed
std::string in_port_kind(const std::string& type, const std::string& port,
                         const organs::template_loader& load) {
  if (const auto* t = native_named(type)) {
    for (const auto& p : t->in_ports)
      if (port == p.name) return p.kind;
    return "";
  }
  if (auto tj = load(type)) {
    auto doc = organs::parse_graph(*tj);
    if (!doc.inlets.contains(port)) return "";
    std::string inner = doc.inlets[port];
    auto slash = inner.find('/');
    std::string itype;
    for (const auto& [id, ty] : doc.nodes)
      if (id == inner.substr(0, slash)) itype = ty;
    return in_port_kind(itype, inner.substr(slash + 1), load);
  }
  return "";
}

}  // namespace

organs::graph_doc lift_expand(organs::graph_doc doc,
                              const organs::template_loader& load) {
  auto type_of = [&](const std::string& id) {
    for (const auto& [nid, t] : doc.nodes)
      if (nid == id) return t;
    return std::string();
  };
  // find span edges: producer's out kind == span
  struct lift_site {
    std::size_t edge;
    std::string target, port;
    nlohmann::ordered_json values;
  };
  std::vector<lift_site> lifts;
  std::set<std::size_t> drop_edges;
  for (std::size_t i = 0; i < doc.edges.size(); ++i) {
    const auto& [from, to] = doc.edges[i];
    auto sid = from.substr(0, from.find('/'));
    const auto* st = native_named(type_of(sid));
    if (!st) continue;
    bool span_out = false;
    for (const auto& p : st->out_ports)
      if (from.substr(from.find('/') + 1) == p.name &&
          std::string(p.kind) == "span")
        span_out = true;
    if (!span_out) continue;
    nlohmann::ordered_json values = nlohmann::ordered_json::array();
    if (doc.defaults.contains(sid + "/values"))
      values = doc.defaults[sid + "/values"];
    auto did = to.substr(0, to.find('/'));
    auto dport = to.substr(to.find('/') + 1);
    auto dkind = in_port_kind(type_of(did), dport, load);
    if (dkind == "span") {
      // whole-by-kind: the consumer takes the span as one unit — no clones
      doc.defaults[did + "/n"] = values.size();
      drop_edges.insert(i);
      continue;
    }
    // excess rank: the consumer lifts — unless it holds a resource
    if (!native_named(type_of(did))) {
      if (auto tj = load(type_of(did))) {
        auto culprit = organs::holder_within(*tj, load);
        if (!culprit.empty())
          throw std::runtime_error(
              "resource holder refuses to lift: " + did + " contains " +
              culprit + " — wire the span to a mixer before the holder");
      }
    } else if (native_named(type_of(did))->resource_holder) {
      throw std::runtime_error("resource holder refuses to lift: " + did);
    }
    lifts.push_back({i, did, dport, values});
    drop_edges.insert(i);
  }
  for (const auto& l : lifts) {
    auto n = l.values.size();
    std::string type = type_of(l.target);
    // clones keyed by index ride the route (EXE-5.2's identity story)
    std::erase_if(doc.nodes, [&](const auto& nd) { return nd.first == l.target; });
    for (std::size_t k = 0; k < n; ++k)
      doc.nodes.emplace_back(l.target + "#" + std::to_string(k), type);
    nlohmann::ordered_json fresh;
    for (const auto& [route, v] : doc.defaults.items()) {
      if (route.starts_with(l.target + "/")) {
        auto rest = route.substr(l.target.size());
        for (std::size_t k = 0; k < n; ++k)
          fresh[l.target + "#" + std::to_string(k) + rest] = v;
      } else {
        fresh[route] = v;
      }
    }
    for (std::size_t k = 0; k < n; ++k)
      fresh[l.target + "#" + std::to_string(k) + "/" + l.port] = l.values[k];
    doc.defaults = std::move(fresh);
    std::vector<std::pair<std::string, std::string>> edges;
    for (std::size_t i = 0; i < doc.edges.size(); ++i) {
      if (drop_edges.count(i)) continue;
      auto [f, t] = doc.edges[i];
      auto fid = f.substr(0, f.find('/'));
      auto tid = t.substr(0, t.find('/'));
      if (fid == l.target) {
        // gathered outputs: legal only into a span consumer
        auto dk = in_port_kind(type_of(tid), t.substr(t.find('/') + 1), load);
        if (dk != "span")
          throw std::runtime_error("excess rank at " + f + " -> " + t +
                                   ": the gathered span needs a span-kind "
                                   "consumer (an explicit mix)");
        for (std::size_t k = 0; k < n; ++k)
          edges.emplace_back(l.target + "#" + std::to_string(k) +
                                 f.substr(l.target.size()), t);
      } else if (tid == l.target) {
        for (std::size_t k = 0; k < n; ++k)
          edges.emplace_back(f, l.target + "#" + std::to_string(k) +
                                    t.substr(l.target.size()));
      } else {
        edges.emplace_back(f, t);
      }
    }
    doc.edges = std::move(edges);
    drop_edges.clear();  // indices no longer valid; single-site per pass
    break;               // re-scan would be needed for multi-lift; one per doc
  }
  if (!lifts.empty() && lifts.size() > 1)
    throw std::runtime_error("one lift site per graph at this rung");
  // drop the consumed span edges when no lift rewrote the edge list
  if (lifts.empty() && !drop_edges.empty()) {
    std::vector<std::pair<std::string, std::string>> edges;
    for (std::size_t i = 0; i < doc.edges.size(); ++i)
      if (!drop_edges.count(i)) edges.push_back(doc.edges[i]);
    doc.edges = std::move(edges);
  }
  return doc;
}

}  // namespace syg::executor
