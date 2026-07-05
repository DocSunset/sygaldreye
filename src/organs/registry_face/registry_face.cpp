// clause: machinery — the registry organ
#include "registry_face/registry_face.hpp"

namespace syg::organs {

nlohmann::json palette() {
  auto out = nlohmann::json::array();
  for (const auto* n : registered_natives()) out.push_back(n->name);
  return out;
}

}  // namespace syg::organs
