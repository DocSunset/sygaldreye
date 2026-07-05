// clause: machinery — the subgraph organ (LNG-6)
#pragma once
#include <functional>
#include <optional>

#include "parser/parser.hpp"

namespace syg::organs {

using template_loader =
    std::function<std::optional<nlohmann::json>(const std::string& type)>;

// Expand every template-typed node in place: clone inner nodes as
// "outer.inner", rewire through inlets/outlets, node-entry defaults land
// on the inner inlet targets. Recursive; throws on template cycles.
graph_doc expand_subgraphs(graph_doc doc, const template_loader& load);

// Resource-holder inference: does this template's closure contain one?
// Returns the inner culprit's route, empty if none.
std::string holder_within(const nlohmann::json& template_json,
                          const template_loader& load);

}  // namespace syg::organs
