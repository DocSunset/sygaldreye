// clause: machinery — mesh package (peer identity, crypto: ADR-035).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace syg::mesh {

using bytes = std::vector<std::uint8_t>;

// A peer's long-term Ed25519 identity (ADR-035). The peer-key is the public
// key, text-encoded exactly as a CID (`#peerkey` address step). Keys may be
// SEEDED so tests are exact-class (same seed -> same key -> same signatures).
class identity {
 public:
  // Deterministic from a seed string (test/boot); the seed hashes to the
  // Ed25519 seed. An empty seed draws from the OS CSPRNG (real peers).
  explicit identity(const std::string& seed = "");

  const bytes& public_key() const { return pk_; }
  std::string peer_key() const;  // multibase base32 of the public key

  bytes sign(const bytes& msg) const;  // detached Ed25519 signature

 private:
  bytes pk_, sk_;
};

// Signature check against a public key (32 bytes). No trust — just math.
bool verify(const bytes& public_key, const bytes& msg, const bytes& sig);

// peer-key text <-> raw public-key bytes.
std::string peer_key_of(const bytes& public_key);
bytes public_key_of(const std::string& peer_key);

}  // namespace syg::mesh
