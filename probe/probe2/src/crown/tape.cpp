#include <stdexcept>

#include "crown.hpp"

namespace syg::crown {
namespace {

// percent-unescape, per the pinned escaping rule (shared grammar w/ FMT-2)
std::string unesc(const std::string& s) {
  std::string out;
  for (std::size_t i = 0; i < s.size(); ++i)
    if (s[i] == '%' && i + 2 < s.size()) {
      out.push_back(static_cast<char>(std::stoi(s.substr(i + 1, 2), nullptr, 16)));
      i += 2;
    } else {
      out.push_back(s[i]);
    }
  return out;
}

}  // namespace

std::vector<op> read_tape(const std::string& text) {
  std::vector<op> ops;
  int lineno = 0;
  std::size_t pos = 0;
  while (pos <= text.size()) {
    auto nl = text.find('\n', pos);
    std::string line = text.substr(pos, nl == std::string::npos ? nl : nl - pos);
    pos = nl == std::string::npos ? text.size() + 1 : nl + 1;
    ++lineno;
    std::vector<std::string> f;
    std::size_t p = 0;
    while (p < line.size()) {
      auto sp = line.find(' ', p);
      auto field = line.substr(p, sp == std::string::npos ? sp : sp - p);
      if (!field.empty()) f.push_back(unesc(field));
      p = sp == std::string::npos ? line.size() : sp + 1;
    }
    if (f.empty() || f[0].starts_with("#")) continue;
    if (f[0] == "NODE" && f.size() == 3)
      ops.push_back({op::kind::instantiate, f[1], f[2], ""});
    else if (f[0] == "LINK" && f.size() == 3)
      ops.push_back({op::kind::link, f[1], f[2], ""});
    else if (f[0] == "SET" && f.size() == 3)
      ops.push_back({op::kind::write_default, f[1], f[2], ""});
    else
      throw std::runtime_error("tape line " + std::to_string(lineno) +
                               ": bad record");
  }
  return ops;
}

}  // namespace syg::crown
