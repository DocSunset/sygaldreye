// clause: machinery — the naive resolver organ
#include "resolver/naive_resolver.hpp"

#include <fstream>
#include <stdexcept>

namespace syg::organs {

formats::byte_vec naive_resolve(const std::string& objdir,
                                const std::string& cid_text) {
  std::ifstream f(objdir + "/" + cid_text, std::ios::binary);
  if (!f) throw std::runtime_error("object miss: " + cid_text);
  formats::byte_vec bytes{std::istreambuf_iterator<char>(f),
                          std::istreambuf_iterator<char>()};
  if (!formats::cid_verify(formats::cid_from_text(cid_text), bytes))
    throw std::runtime_error("verification failed: " + cid_text +
                             " does not hash to itself");
  return bytes;
}

}  // namespace syg::organs
