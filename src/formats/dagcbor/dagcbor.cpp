#include "dagcbor.hpp"

#include "cid/cid.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace syg::formats {
namespace {

using bytes = std::vector<std::uint8_t>;

void head(bytes& out, unsigned major, std::uint64_t arg) {
  if (arg < 24) {
    out.push_back(static_cast<std::uint8_t>(major << 5 | arg));
    return;
  }
  int extra = arg < (1u << 8) ? 1 : arg < (1u << 16) ? 2 : arg < (1ull << 32) ? 4 : 8;
  static constexpr std::uint8_t ai[9] = {0, 24, 25, 0, 26, 0, 0, 0, 27};
  out.push_back(static_cast<std::uint8_t>(major << 5 | ai[extra]));
  for (int i = extra - 1; i >= 0; --i)
    out.push_back(static_cast<std::uint8_t>(arg >> (8 * i)));
}

bytes base64_decode(const std::string& s) {
  static constexpr char alphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  bytes out;
  unsigned buf = 0, nbits = 0;
  for (char c : s) {
    const char* p = static_cast<const char*>(std::memchr(alphabet, c, 64));
    if (!p) throw std::runtime_error("bad base64 in bytes projection");
    buf = buf << 6 | static_cast<unsigned>(p - alphabet);
    nbits += 6;
    if (nbits >= 8) {
      nbits -= 8;
      out.push_back(static_cast<std::uint8_t>(buf >> nbits & 0xff));
    }
  }
  return out;
}

void encode(bytes& out, const nlohmann::json& v) {
  using nlohmann::json;
  switch (v.type()) {
    case json::value_t::null:
      out.push_back(0xf6);
      return;
    case json::value_t::boolean:
      out.push_back(v.get<bool>() ? 0xf5 : 0xf4);
      return;
    case json::value_t::number_unsigned:
      head(out, 0, v.get<std::uint64_t>());
      return;
    case json::value_t::number_integer: {
      auto i = v.get<std::int64_t>();
      if (i >= 0)
        head(out, 0, static_cast<std::uint64_t>(i));
      else
        head(out, 1, static_cast<std::uint64_t>(-1 - i));
      return;
    }
    case json::value_t::number_float: {
      double d = v.get<double>();
      if (std::isnan(d) || std::isinf(d))
        throw std::runtime_error("NaN/Inf forbidden in dag-cbor");
      out.push_back(0xfb);
      auto u = std::bit_cast<std::uint64_t>(d);
      for (int i = 7; i >= 0; --i)
        out.push_back(static_cast<std::uint8_t>(u >> (8 * i)));
      return;
    }
    case json::value_t::string: {
      const auto& s = v.get_ref<const std::string&>();
      head(out, 3, s.size());
      out.insert(out.end(), s.begin(), s.end());
      return;
    }
    case json::value_t::array: {
      head(out, 4, v.size());
      for (const auto& x : v) encode(out, x);
      return;
    }
    case json::value_t::object: {
      if (v.size() == 1 && v.contains("/")) {  // DAG-JSON escapes
        const auto& esc = v["/"];
        if (esc.is_string()) {  // link: tag 42 over 0x00 + cid bytes
          auto cid = cid_from_text(esc.get_ref<const std::string&>());
          head(out, 6, 42);
          head(out, 2, cid.size() + 1);
          out.push_back(0x00);
          out.insert(out.end(), cid.begin(), cid.end());
          return;
        }
        if (esc.is_object() && esc.size() == 1 && esc.contains("bytes") &&
            esc["bytes"].is_string()) {
          bytes b = base64_decode(esc["bytes"].get_ref<const std::string&>());
          head(out, 2, b.size());
          out.insert(out.end(), b.begin(), b.end());
          return;
        }
        throw std::runtime_error("malformed \"/\" projection escape");
      }
      std::vector<std::pair<std::string, const nlohmann::json*>> items;
      for (auto it = v.begin(); it != v.end(); ++it) {
        if (it.key() == "/")
          throw std::runtime_error("map key \"/\" is reserved by the projection");
        items.emplace_back(it.key(), &it.value());
      }
      std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
        return a.first.size() != b.first.size() ? a.first.size() < b.first.size()
                                                : a.first < b.first;
      });
      head(out, 5, items.size());
      for (const auto& [k, val] : items) {
        head(out, 3, k.size());
        out.insert(out.end(), k.begin(), k.end());
        encode(out, *val);
      }
      return;
    }
    default:
      throw std::runtime_error("unencodable value");
  }
}

}  // namespace

bytes encode_projection(const nlohmann::json& v) {
  bytes out;
  encode(out, v);
  return out;
}

bytes bytes_of_projection(const nlohmann::json& v) {
  if (v.is_object() && v.size() == 1 && v.contains("/") && v["/"].is_object() &&
      v["/"].contains("bytes") && v["/"]["bytes"].is_string())
    return base64_decode(v["/"]["bytes"].get_ref<const std::string&>());
  throw std::runtime_error("expected a bytes projection escape");
}

}  // namespace syg::formats
