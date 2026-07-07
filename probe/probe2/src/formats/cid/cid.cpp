#include "cid.hpp"

#include <blake3.h>

#include <stdexcept>

#include "pins/pins.hpp"

namespace syg::formats {
namespace {

constexpr std::string_view b32 = "abcdefghijklmnopqrstuvwxyz234567";

void put_varint(byte_vec& out, std::uint64_t v) {
  while (v >= 0x80) {
    out.push_back(static_cast<std::uint8_t>(v | 0x80));
    v >>= 7;
  }
  out.push_back(static_cast<std::uint8_t>(v));
}

}  // namespace

byte_vec blake3_256(const std::uint8_t* data, std::size_t len) {
  blake3_hasher h;
  blake3_hasher_init(&h);
  blake3_hasher_update(&h, data, len);
  byte_vec out(pins::hash_bytes);
  blake3_hasher_finalize(&h, out.data(), out.size());
  return out;
}

byte_vec cid_of(std::uint64_t multicodec, const byte_vec& data) {
  byte_vec cid{0x01};
  put_varint(cid, multicodec);
  put_varint(cid, pins::multihash_blake3_256);
  put_varint(cid, pins::hash_bytes);
  auto h = blake3_256(data.data(), data.size());
  cid.insert(cid.end(), h.begin(), h.end());
  return cid;
}

std::string mb32_encode(const byte_vec& data) {
  std::string out(1, pins::multibase_prefix);
  unsigned buf = 0, nbits = 0;
  for (auto b : data) {
    buf = buf << 8 | b;
    nbits += 8;
    while (nbits >= 5) {
      nbits -= 5;
      out.push_back(b32[buf >> nbits & 0x1f]);
    }
  }
  if (nbits) out.push_back(b32[buf << (5 - nbits) & 0x1f]);
  return out;
}

byte_vec mb32_decode(const std::string& text) {
  if (text.empty() || text[0] != pins::multibase_prefix)
    throw std::runtime_error("unsupported multibase: " + text.substr(0, 1));
  byte_vec out;
  unsigned buf = 0, nbits = 0;
  for (char c : text.substr(1)) {
    auto pos = b32.find(c);
    if (pos == std::string_view::npos) throw std::runtime_error("bad base32");
    buf = buf << 5 | static_cast<unsigned>(pos);
    nbits += 5;
    if (nbits >= 8) {
      nbits -= 8;
      out.push_back(static_cast<std::uint8_t>(buf >> nbits & 0xff));
    }
  }
  return out;
}

std::string cid_to_text(const byte_vec& cid) { return mb32_encode(cid); }

byte_vec cid_from_text(const std::string& text) { return mb32_decode(text); }

bool cid_verify(const byte_vec& cid, const byte_vec& data) {
  if (cid.size() < 4 || cid[0] != 0x01) return false;
  std::size_t i = 1;
  auto varint = [&](std::uint64_t& v) {
    v = 0;
    for (int shift = 0; i < cid.size(); shift += 7) {
      std::uint8_t b = cid[i++];
      v |= static_cast<std::uint64_t>(b & 0x7f) << shift;
      if (!(b & 0x80)) return true;
    }
    return false;
  };
  std::uint64_t codec, hash_fn, hash_len;
  if (!varint(codec) || !varint(hash_fn) || !varint(hash_len)) return false;
  if (hash_fn != pins::multihash_blake3_256 || hash_len != pins::hash_bytes ||
      cid.size() - i != pins::hash_bytes)
    return false;
  auto h = blake3_256(data.data(), data.size());
  return std::equal(h.begin(), h.end(), cid.begin() + static_cast<long>(i));
}

}  // namespace syg::formats
