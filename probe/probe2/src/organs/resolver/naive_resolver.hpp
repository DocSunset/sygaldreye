// clause: machinery — the naive resolver organ (SZ-3)
#pragma once
#include <string>

#include "cid/cid.hpp"

namespace syg::organs {

// hash -> verified bytes from the object directory; throws on miss or
// verification failure (a corrupt object never leaves this function).
formats::byte_vec naive_resolve(const std::string& objdir,
                                const std::string& cid_text);

}  // namespace syg::organs
