// clause: machinery — libsodium is the maturity import (ADR-033 clause 3).
#include "identity.hpp"

#include <sodium.h>

#include <stdexcept>

#include "cid/cid.hpp"

namespace syg::mesh {
namespace {

struct sodium_boot {
  sodium_boot() {
    if (sodium_init() < 0) throw std::runtime_error("libsodium init failed");
  }
};
void ensure_sodium() { static sodium_boot once; (void)once; }

formats::byte_vec to_fmt(const bytes& b) { return {b.begin(), b.end()}; }
bytes from_fmt(const formats::byte_vec& b) { return {b.begin(), b.end()}; }

}  // namespace

identity::identity(const std::string& seed) {
  ensure_sodium();
  pk_.resize(crypto_sign_PUBLICKEYBYTES);
  sk_.resize(crypto_sign_SECRETKEYBYTES);
  if (seed.empty()) {
    crypto_sign_keypair(pk_.data(), sk_.data());
  } else {
    // seed -> a fixed 32-byte Ed25519 seed by hashing (any length seed works)
    bytes s(crypto_sign_SEEDBYTES);
    crypto_generichash(s.data(), s.size(),
                       reinterpret_cast<const unsigned char*>(seed.data()),
                       seed.size(), nullptr, 0);
    crypto_sign_seed_keypair(pk_.data(), sk_.data(), s.data());
  }
}

std::string identity::peer_key() const { return peer_key_of(pk_); }

bytes identity::sign(const bytes& msg) const {
  bytes sig(crypto_sign_BYTES);
  crypto_sign_detached(sig.data(), nullptr, msg.data(), msg.size(), sk_.data());
  return sig;
}

bool verify(const bytes& public_key, const bytes& msg, const bytes& sig) {
  ensure_sodium();
  if (public_key.size() != crypto_sign_PUBLICKEYBYTES ||
      sig.size() != crypto_sign_BYTES)
    return false;
  return crypto_sign_verify_detached(sig.data(), msg.data(), msg.size(),
                                     public_key.data()) == 0;
}

std::string peer_key_of(const bytes& public_key) {
  return formats::mb32_encode(to_fmt(public_key));
}

bytes public_key_of(const std::string& peer_key) {
  return from_fmt(formats::mb32_decode(peer_key));
}

}  // namespace syg::mesh
