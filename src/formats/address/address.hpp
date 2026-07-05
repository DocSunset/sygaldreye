#pragma once
#include <string>
#include <vector>

namespace syg::formats {

// A parsed address (ch. 14, ADR-029): the first step's spelling kind, the
// first step, and the remaining route of container-conferred local names.
struct address {
  enum class first_step { refname, cid, peerkey };
  first_step kind;
  std::string head;
  std::vector<std::string> route;
};

address parse_address(const std::string& text);

}  // namespace syg::formats
