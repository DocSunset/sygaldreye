#pragma once
#include <functional>

#include "cid/cid.hpp"

namespace syg::formats {

// Store a blob through the pinned chunking rule (ch. 14): at or below the
// chunk size it is one raw object; above, fixed-size raw chunks plus a
// dag-cbor index node of links. `put` receives every object exactly once
// per call (the sink dedups by cid). Returns the root cid.
byte_vec chunk_put(const byte_vec& blob,
                   const std::function<void(const byte_vec& cid,
                                            const byte_vec& object)>& put);

}  // namespace syg::formats
