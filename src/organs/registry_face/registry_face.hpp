// clause: machinery — the registry organ (readable as the palette)
#pragma once
#include <vector>

#include <nlohmann/json.hpp>

#include "crown.hpp"

namespace syg::organs {

// Defined by the GENERATED registration TU (SZ-2): the linked vocabulary.
const std::vector<const syg::crown::native_type*>& registered_natives();

// The face: the palette as data.
nlohmann::json palette();

}  // namespace syg::organs
