// clause: floor — kernel natives over ugens (their registration tables fold
// into SZ-2's generated TU at rung 4)
#pragma once
#include <vector>

#include "crown.hpp"

namespace syg::nodes {

// The hello-cosine vocabulary as linked natives: osc, lfo, vca (floor) and
// the placeholder dac sink (see dac_native.cpp).
const std::vector<const syg::crown::native_type*>& hello_natives();

}  // namespace syg::nodes
