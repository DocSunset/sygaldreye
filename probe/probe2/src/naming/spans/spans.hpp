#pragma once
#include <cstddef>
#include <string>

#include "environment/environment.hpp"

namespace syg::naming {

// A hash-identified span: version-independent, edit-stable (NAM-7).
struct span {
  std::string piece_cid;
  std::size_t start = 0;  // within the piece
  std::size_t len = 0;
};

struct located {
  std::string text;
  std::size_t position = 0;  // absolute in the given version — derived
};

// Mint a span covering [pos, pos+len) of a text node's current content.
span span_at(environment& env, const nlohmann::json& text_node,
             std::size_t pos, std::size_t len);

// Resolve a span against a version: same characters, position derived.
located span_text(environment& env, const nlohmann::json& text_node,
                  const span& s);

}  // namespace syg::naming
