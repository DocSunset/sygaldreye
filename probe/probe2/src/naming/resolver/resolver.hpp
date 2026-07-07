#pragma once
#include <nlohmann/json.hpp>

#include "address/address.hpp"
#include "environment/environment.hpp"

namespace syg::naming {

struct resolution {
  bool live = false;        // crossed a ref (NAM-2)
  std::string normalized;   // the fixed, cid-rooted spelling
  nlohmann::json value;     // projection of the reached value
};

resolution resolve(environment& env, const formats::address& addr);

}  // namespace syg::naming
