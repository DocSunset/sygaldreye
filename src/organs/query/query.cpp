// clause: machinery — the query organ (see query.hpp; the walk dissolved)
#include "query/query.hpp"

#include <functional>

namespace syg::organs {

std::set<std::string> query_step(const store::peer_store& s,
                                 const std::set<std::string>& from,
                                 const std::string& via) {
  std::set<std::string> out;
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
                  out.insert(*r);  // the conferred-mutable hop
              }
            };
        walk(node);
      } catch (const std::exception&) {
      }
    }
  }
  return out;
}

std::set<std::string> query_outputs_of(const store::peer_store& s,
                                       const std::set<std::string>& records) {
  std::set<std::string> out;
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

}  // namespace syg::organs
