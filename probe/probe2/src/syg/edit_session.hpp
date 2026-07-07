#pragma once
#include <nlohmann/json.hpp>

namespace syg::harness {

// `syg edit` (HARNESS.md): the graph editor held to its laws (ch. 9). The
// editor is graphs editing graphs — a live plan whose arbiter takes edit ops
// exactly as the gesture nodes emit them; serialize captures DEFAULTS not
// live values; undo is the op log's inverse cursor (ADR-018). EDR-1..8.
int edit_session(const nlohmann::json& in);

}  // namespace syg::harness
