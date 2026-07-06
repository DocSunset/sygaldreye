#include "document_session.hpp"

#include <iostream>
#include <string>
#include <vector>

#include "store.hpp"

namespace syg::harness {
namespace {

using nlohmann::json;

// Decompose C++ source into TOP-LEVEL segments: a real structural split at
// depth-0 terminators (`;`, a closing `}`, or a preprocessor line end),
// keeping trailing whitespace with each segment. The partition covers every
// byte exactly once and in order, so regeneration (concatenation) is
// byte-identical by construction — the honest round-trip (EDR-6.2).
std::vector<std::string> decompose(const std::string& src) {
  std::vector<std::string> segs;
  std::size_t start = 0, i = 0, n = src.size();
  int depth = 0;
  bool preproc = false;
  auto flush_ws_then_cut = [&](std::size_t end) {
    // absorb trailing spaces/newlines into this segment so the seams carry
    // no bytes of their own
    while (end < n && (src[end] == ' ' || src[end] == '\t' ||
                       src[end] == '\n' || src[end] == '\r'))
      ++end;
    segs.push_back(src.substr(start, end - start));
    start = end;
    i = end;
  };
  while (i < n) {
    char c = src[i];
    if (c == '#' && (i == 0 || src[i - 1] == '\n')) preproc = true;
    if (c == '/' && i + 1 < n && src[i + 1] == '/') {  // line comment
      while (i < n && src[i] != '\n') ++i;
      continue;
    }
    if (c == '/' && i + 1 < n && src[i + 1] == '*') {  // block comment
      i += 2;
      while (i + 1 < n && !(src[i] == '*' && src[i + 1] == '/')) ++i;
      i += 2;
      continue;
    }
    if (c == '"' || c == '\'') {  // string / char literal
      char q = c;
      ++i;
      while (i < n && src[i] != q) {
        if (src[i] == '\\') ++i;
        ++i;
      }
      ++i;
      continue;
    }
    if (c == '{' || c == '(' || c == '[') ++depth;
    else if (c == '}' || c == ')' || c == ']') --depth;
    if (preproc && c == '\n') {  // a preprocessor directive is one segment
      preproc = false;
      flush_ws_then_cut(i + 1);
      continue;
    }
    if (depth == 0 && (c == ';' || c == '}')) {  // a top-level construct ends
      flush_ws_then_cut(i + 1);
      continue;
    }
    ++i;
  }
  if (start < n) segs.push_back(src.substr(start));  // trailing tail
  return segs;
}

}  // namespace

int document_session(const nlohmann::json& in) {
  store::peer_store s("documents");
  json results = json::array();
  for (const auto& op : in.value("ops", json::array())) {
    const std::string what = op.at("op");
    json r;
    if (what == "roundtrip") {
      const std::string src = op.at("source");
      auto segs = decompose(src);
      // commit the document: segment nodes linked in sequence (a graph, ch. 9)
      json nodes = json::array();
      std::string prev;
      for (std::size_t k = 0; k < segs.size(); ++k) {
        json seg{{"kind", "segment"}, {"ord", k}, {"text", segs[k]}};
        if (!prev.empty()) seg["prev"] = {{"/", prev}};
        prev = s.put_node(seg, false);
        nodes.push_back(prev);
      }
      auto doc_cid = s.put_node(
          {{"kind", "document"}, {"language", "cpp"}, {"segments", nodes}}, false);
      // regenerate by walking the segments in order
      std::string regen;
      for (const auto& cid : nodes)
        regen += s.get_node(cid.get<std::string>()).at("text").get<std::string>();
      r = {{"segments", segs.size()}, {"document", doc_cid},
           {"identical", regen == src}, {"regen_len", regen.size()}};
    } else {
      r = {{"error", "unknown op"}, {"op", what}};
    }
    results.push_back(r);
  }
  std::cout << json{{"results", results}}.dump() << "\n";
  return 0;
}

}  // namespace syg::harness
