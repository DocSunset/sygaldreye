#include "address.hpp"

#include <stdexcept>

#include "pins/pins.hpp"

namespace syg::formats {
namespace {

std::vector<std::string> split(const std::string& s, char sep) {
  std::vector<std::string> out;
  std::size_t start = 0;
  for (std::size_t i = 0; i <= s.size(); ++i)
    if (i == s.size() || s[i] == sep) {
      out.push_back(s.substr(start, i - start));
      start = i + 1;
    }
  return out;
}

std::string unescape(const std::string& s) {
  std::string out;
  for (std::size_t i = 0; i < s.size(); ++i)
    if (s[i] == '%') {
      if (i + 2 >= s.size()) throw std::runtime_error("truncated %-escape");
      out.push_back(static_cast<char>(std::stoi(s.substr(i + 1, 2), nullptr, 16)));
      i += 2;
    } else {
      out.push_back(s[i]);
    }
  return out;
}

// Unescape each segment; drop empties (the cid/peerkey spellings ignore
// stray slashes; the refname spelling keeps whatever the route says).
std::vector<std::string> steps_of(const std::vector<std::string>& segs,
                                  bool drop_empty) {
  std::vector<std::string> out;
  for (const auto& s : segs)
    if (!drop_empty || !s.empty()) out.push_back(unescape(s));
  return out;
}

std::string escape(const std::string& s) {
  std::string out;
  for (char c : s)
    if (pins::escape_set.find(c) != std::string_view::npos) {
      static constexpr char hex[] = "0123456789ABCDEF";
      out += {'%', hex[(c >> 4) & 0xf], hex[c & 0xf]};
    } else {
      out.push_back(c);
    }
  return out;
}

}  // namespace

std::string print_address(const address& a) {
  std::string route;
  for (const auto& s : a.route) route += (route.empty() ? "" : "/") + escape(s);
  switch (a.kind) {
    case address::first_step::peerkey:
      return "#" + escape(a.head) + (route.empty() ? "" : "/" + route);
    case address::first_step::refname:
      return escape(a.head) + (route.empty() ? "" : ":" + route);
    default:
      return escape(a.head) + (route.empty() ? "" : "/" + route);
  }
}

address parse_address(const std::string& text) {
  if (!text.empty() && text[0] == '#') {  // peer-key spelling
    auto segs = split(text.substr(1), '/');
    auto head = segs[0];
    segs.erase(segs.begin());
    return {address::first_step::peerkey, unescape(head), steps_of(segs, true)};
  }
  auto first_seg = text.substr(0, text.find('/'));
  auto colon = first_seg.find(':');
  if (colon != std::string::npos) {  // lexical refname spelling
    auto route = text.substr(colon + 1);
    return {address::first_step::refname, unescape(text.substr(0, colon)),
            route.empty() ? std::vector<std::string>{}
                          : steps_of(split(route, '/'), false)};
  }
  auto segs = split(text, '/');  // cid spelling (heuristic shared with oracle)
  auto head = segs[0];
  segs.erase(segs.begin());
  auto kind = head.size() > 8 && head[0] == 'b' ? address::first_step::cid
                                                : address::first_step::refname;
  return {kind, unescape(head), steps_of(segs, true)};
}

}  // namespace syg::formats
