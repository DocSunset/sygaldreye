#include "resolver.hpp"

#include <stdexcept>

namespace syg::naming {
namespace {

bool is_link(const nlohmann::json& v) {
  return v.is_object() && v.size() == 1 && v.contains("/") && v["/"].is_string();
}

const std::string& link_cid(const nlohmann::json& v) {
  return v["/"].get_ref<const std::string&>();
}

// Follow a member value: links fetch on demand.
const nlohmann::json& follow(environment& env, const nlohmann::json& v) {
  return is_link(v) ? env.fetch(link_cid(v)) : v;
}

}  // namespace

resolution resolve(environment& env, const formats::address& addr) {
  resolution res;
  std::string root_cid;
  switch (addr.kind) {
    case formats::address::first_step::refname: {
      const auto* cid = env.ref(addr.head);
      if (!cid) throw std::runtime_error("no such ref: " + addr.head);
      root_cid = *cid;
      res.live = true;  // the one conferred-mutable step
      break;
    }
    case formats::address::first_step::cid:
      root_cid = addr.head;
      break;
    default:
      throw std::runtime_error("peer-key steps arrive with the mesh (rung 9)");
  }
  const nlohmann::json* cur = &env.fetch(root_cid);
  std::string lock_cid;  // set while descending a graph's nodes
  for (const auto& step : addr.route) {
    if (!cur->is_object())
      throw std::runtime_error("cannot step into a leaf at '" + step + "'");
    if (cur->value("kind", "") == "graph" && step == "nodes") {
      lock_cid = link_cid(cur->at("lock"));
      cur = &follow(env, cur->at("topology")).at("nodes");
      continue;
    }
    if (cur->contains(step)) {
      cur = &cur->at(step);
      if (is_link(*cur) && &step != &addr.route.back()) cur = &follow(env, *cur);
      continue;
    }
    if (cur->contains("type") && !lock_cid.empty()) {
      // a topology node record: port steps are answered by the type
      // declaration, reached through the graph's vocabulary lock (ch. 2)
      const auto& lock = env.fetch(lock_cid);
      const auto& decl =
          env.fetch(link_cid(lock.at(cur->at("type").get<std::string>())));
      cur = &decl.at("ports").at(step);
      continue;
    }
    throw std::runtime_error("no step '" + step + "' here");
  }
  res.normalized = formats::print_address(
      {formats::address::first_step::cid, root_cid, addr.route});
  res.value = *cur;
  return res;
}

}  // namespace syg::naming
