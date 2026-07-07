#include "spans.hpp"

#include <stdexcept>

namespace syg::naming {
namespace {

// Walk the pieces once, calling f(piece_cid, piece_text, absolute_offset).
template <class F>
void walk(environment& env, const nlohmann::json& text_node, F&& f) {
  std::size_t off = 0;
  for (const auto& link : text_node.at("pieces")) {
    const auto& cid = link.at("/").get_ref<const std::string&>();
    const auto& piece = env.fetch(cid).get_ref<const std::string&>();
    f(cid, piece, off);
    off += piece.size();
  }
}

}  // namespace

span span_at(environment& env, const nlohmann::json& text_node,
             std::size_t pos, std::size_t len) {
  span out;
  walk(env, text_node, [&](const std::string& cid, const std::string& piece,
                           std::size_t off) {
    if (out.len == 0 && pos >= off && pos + len <= off + piece.size())
      out = {cid, pos - off, len};
  });
  if (out.len == 0) throw std::runtime_error("span does not fit one piece");
  return out;
}

located span_text(environment& env, const nlohmann::json& text_node,
                  const span& s) {
  located out;
  bool found = false;
  walk(env, text_node, [&](const std::string& cid, const std::string& piece,
                           std::size_t off) {
    if (!found && cid == s.piece_cid) {
      out = {piece.substr(s.start, s.len), off + s.start};
      found = true;
    }
  });
  if (!found) throw std::runtime_error("span's piece is not in this version");
  return out;
}

}  // namespace syg::naming
