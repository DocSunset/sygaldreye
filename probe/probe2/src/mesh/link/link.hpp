// clause: machinery — mesh package (authenticated encrypted transport).
// ADR-035: Ed25519 identity handshake -> crypto_kx session keys ->
// XChaCha20-Poly1305 secretstream framing over a loopback TCP socket.
#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "identity/identity.hpp"

namespace syg::mesh {

// One framed, authenticated, encrypted duplex channel. Each message is a
// varint wire-kind + dag-cbor body, carried as one secretstream message.
class channel {
 public:
  channel(int fd, void* streams, bytes remote_pk);
  ~channel();
  channel(channel&&) = delete;
  channel& operator=(channel&&) = delete;

  void send(std::uint64_t kind, const bytes& body);
  bool recv(std::uint64_t& kind, bytes& body);  // false at clean EOF/close
  const bytes& remote_pk() const { return remote_pk_; }
  std::string remote_peer_key() const { return peer_key_of(remote_pk_); }

 private:
  int fd_;
  void* streams_;  // owned crypto_secretstream push/pull pair
  bytes remote_pk_;
};

// A bound loopback listener (127.0.0.1:ephemeral).
class listener {
 public:
  listener();
  ~listener();
  std::uint16_t port() const { return port_; }

  // `admit(peer_key) -> bool` is consulted at handshake time (so revocation
  // mid-session takes effect immediately, MSH-1). The peer owns the pairing
  // table and guards it.
  using admit_fn = std::function<bool(const std::string&)>;

  // Accept one connection and run the responder handshake as `me`, admitting
  // only initiators `admit` approves. Returns a channel on success; nullptr on
  // refusal, garbage, or EOF (the socket is closed silently — no plaintext
  // service surface, MSH-2).
  std::unique_ptr<channel> accept(const identity& me, const admit_fn& admit);

  // The probe surface (MSH-2.1): accept a raw TCP connection and return the
  // fd unmodified, so a test can send HTTP/garbage and observe the refusal.
  int accept_raw();

  // Unblock a thread parked in accept() so a serve loop can exit.
  void stop();

 private:
  int fd_ = -1;
  std::uint16_t port_ = 0;
};

// Dial a listener and run the initiator handshake as `me`; the responder must
// admit us and we must admit it (`admit` approves its key). nullptr on refusal.
std::unique_ptr<channel> dial(std::uint16_t port, const identity& me,
                              const listener::admit_fn& admit);

// A raw, unauthenticated TCP probe (MSH-2.1): connect, send `payload`, read
// whatever the peer replies (bounded). A plaintext service would answer; the
// mesh peer reads a framed hello, sees garbage, and closes — so a conformant
// peer returns zero bytes. Returns the bytes received before EOF/close.
bytes probe(std::uint16_t port, const bytes& payload);

}  // namespace syg::mesh
