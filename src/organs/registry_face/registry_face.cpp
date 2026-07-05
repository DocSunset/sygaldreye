// clause: machinery — the registry organ
#include "registry_face/registry_face.hpp"

namespace syg::organs {

namespace {
std::vector<const syg::crown::native_type*>& loaded() {
  static std::vector<const syg::crown::native_type*> v;
  return v;
}
}  // namespace

void register_plugin_native(const syg::crown::native_type* n) {
  loaded().push_back(n);
}

std::vector<const syg::crown::native_type*> all_natives() {
  auto v = registered_natives();
  v.insert(v.end(), loaded().begin(), loaded().end());
  return v;
}

nlohmann::json palette() {
  auto out = nlohmann::json::array();
  for (const auto* n : all_natives()) out.push_back(n->name);
  return out;
}

}  // namespace syg::organs
