// clause: machinery — the slot organ (EXE-5 swap + migrate)
#pragma once
#include <vector>

#include "crown.hpp"

namespace syg::organs {

// Replace the plan's graph with the new build ops, migrating state by route.
void slot_swap(crown::plan& p, const std::vector<crown::op>& build);

}  // namespace syg::organs
