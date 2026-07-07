#pragma once
#include <nlohmann/json.hpp>

namespace syg::harness {

// syg frame (PKG-4.3): {graph, frames, size:[w,h], ops:[{frame,route,value}]}
// -> N raw RGBA8 frames on stdout; {frames, ns_per_frame} on stderr.
int frame_session(const nlohmann::json& in);

}  // namespace syg::harness
