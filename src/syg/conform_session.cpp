#include "conform_session.hpp"

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "store.hpp"

namespace syg::harness {
namespace {

using nlohmann::json;
using store::peer_store;

// Walk a succession node back to its origin, returning the chain origin->node
// (each entry is the node's committed record).
std::vector<json> chain_to_origin(const peer_store& s, const std::string& cid) {
  std::vector<json> back;
  std::string cur = cid;
  while (!cur.empty()) {
    auto node = s.get_node(cur);
    back.push_back(node);
    cur = node.contains("supersedes") && !node["supersedes"].is_null()
              ? node["supersedes"]["/"].get<std::string>()
              : std::string();
  }
  std::vector<json> fwd(back.rbegin(), back.rend());
  return fwd;
}

// Derived version (ADR-032): M breaking hops from origin, m additive since the
// last breaking, p fix since the last additive. Numbers are stored NOWHERE —
// computed from the chain, so they can never disagree with history.
std::string derive_version(const std::vector<json>& chain) {
  int M = 0, m = 0, p = 0;
  for (const auto& node : chain) {
    const std::string c = node.value("class", "origin");
    if (c == "breaking") { ++M; m = 0; p = 0; }
    else if (c == "additive") { ++m; p = 0; }
    else if (c == "fix") { ++p; }
    // "origin" contributes nothing
  }
  return std::to_string(M) + "." + std::to_string(m) + "." + std::to_string(p);
}

// The green criteria a chain has accumulated (a criterion stays green unless a
// later succession regresses it) — the predecessor baseline the class gate
// checks additive/fix successions against.
std::map<std::string, bool> accumulated_criteria(const std::vector<json>& chain) {
  std::map<std::string, bool> acc;
  for (const auto& node : chain) {
    const json crit = node.value("criteria", json::object());
    for (auto it = crit.begin(); it != crit.end(); ++it)
      acc[it.key()] = it.value().get<bool>();
  }
  return acc;
}

}  // namespace

int conform_session(const nlohmann::json& in) {
  peer_store s("conform");
  std::map<std::string, std::string> heads;  // name -> latest cid in its chain
  json results = json::array();

  for (const auto& op : in.value("ops", json::array())) {
    const std::string what = op.at("op");
    json r;
    if (what == "succeed") {
      // declare a succession: {name, of?, class, criteria?}. Classes are
      // VERIFIED, not vowed (ADR-032): an additive/fix succession that
      // regresses a predecessor's green criterion is REJECTED, naming it.
      const std::string name = op.at("name"), cls = op.value("class", "origin");
      json node{{"kind", "succession"}, {"name", name}, {"class", cls},
                {"criteria", op.value("criteria", json::object())}};
      std::string of;
      if (op.contains("of")) {
        of = heads.at(op.at("of").get<std::string>());  // supersede its head
        node["supersedes"] = {{"/", of}};
      }
      if ((cls == "additive" || cls == "fix") && !of.empty()) {
        auto base = accumulated_criteria(chain_to_origin(s, of));
        const auto& here = node["criteria"];
        for (const auto& [id, was_green] : base)
          if (was_green && here.contains(id) && !here[id].get<bool>())
            throw std::runtime_error(
                "class '" + cls + "' succession regresses a green predecessor "
                "criterion: " + id);
      }
      auto cid = s.put_node(node, false);
      heads[name] = cid;
      r = {{"cid", cid}, {"version", derive_version(chain_to_origin(s, cid))}};
    } else if (what == "version") {
      std::string cid = op.contains("cid") ? op.at("cid").get<std::string>()
                                           : heads.at(op.at("name"));
      r = {{"version", derive_version(chain_to_origin(s, cid))}};
    } else if (what == "resolve") {
      // ref-sugar `name@M.m.p` -> the hash reached by walking the chain to the
      // node whose derived version matches (CNF-6.3).
      const std::string name = op.at("name"), at = op.at("at");
      std::string found;
      auto chain = chain_to_origin(s, heads.at(name));
      for (std::size_t i = 0; i < chain.size(); ++i) {
        std::vector<json> prefix(chain.begin(), chain.begin() + i + 1);
        if (derive_version(prefix) == at)
          found = s.put_node(chain[i], false);  // deterministic: same bytes->same cid
      }
      if (found.empty())
        throw std::runtime_error("no node at " + name + "@" + at);
      r = {{"cid", found}, {"resolved", name + "@" + at}};
    } else {
      r = {{"error", "unknown op"}, {"op", what}};
    }
    results.push_back(r);
  }
  std::cout << json{{"results", results}}.dump() << "\n";
  return 0;
}

}  // namespace syg::harness
