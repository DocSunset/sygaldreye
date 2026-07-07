// clause: machinery — the registry organ (readable as the palette)
#pragma once
#include <vector>

#include <nlohmann/json.hpp>

#include "crown.hpp"

namespace syg::organs {

// Defined by the GENERATED registration TU (SZ-2): the linked vocabulary.
const std::vector<const syg::crown::native_type*>& registered_natives();

// The plugin gate's MECHANICAL half (AUT-5; MSH-5 adds trust): loaded
// node types join the registry view. all_natives() = linked + loaded —
// the four authoring routes are palette-identical through it.
void register_plugin_native(const syg::crown::native_type* n);
std::vector<const syg::crown::native_type*> all_natives();

// The face: the palette as data.
nlohmann::json palette();

}  // namespace syg::organs
