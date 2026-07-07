// clause: machinery — POSIX sockets + libsodium. The one place that speaks
// the wire (ADR-035). Kept small: raw framing helpers + the handshake.
#include "link.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sodium.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>

namespace syg::mesh {
namespace {

constexpr std::uint32_t kMaxFrame = 1u << 20;  // 1 MiB per message

bool read_n(int fd, void* buf, std::size_t n) {
  auto* p = static_cast<std::uint8_t*>(buf);
  while (n) {
    auto r = ::read(fd, p, n);
    if (r <= 0) return false;
    p += r;
    n -= static_cast<std::size_t>(r);
  }
  return true;
}

bool write_n(int fd, const void* buf, std::size_t n) {
  const auto* p = static_cast<const std::uint8_t*>(buf);
  while (n) {
    // MSG_NOSIGNAL: a peer that closed mid-write yields EPIPE, not a signal.
    auto r = ::send(fd, p, n, MSG_NOSIGNAL);
    if (r <= 0) return false;
    p += r;
    n -= static_cast<std::size_t>(r);
  }
  return true;
}

// A length-delimited plaintext frame (handshake only): 4-byte BE length +
// body. An absurd length (HTTP verbs, random scans) fails the bound and the
// socket closes with no reply.
bool read_frame(int fd, bytes& out) {
  std::uint8_t len[4];
  if (!read_n(fd, len, 4)) return false;
  std::uint32_t n = std::uint32_t(len[0]) << 24 | std::uint32_t(len[1]) << 16 |
                    std::uint32_t(len[2]) << 8 | std::uint32_t(len[3]);
  if (n > kMaxFrame) return false;
  out.resize(n);
  return read_n(fd, out.data(), n);
}

bool write_frame(int fd, const bytes& body) {
  std::uint32_t n = static_cast<std::uint32_t>(body.size());
  std::uint8_t len[4] = {std::uint8_t(n >> 24), std::uint8_t(n >> 16),
                         std::uint8_t(n >> 8), std::uint8_t(n)};
  return write_n(fd, len, 4) && write_n(fd, body.data(), body.size());
}

void put_varint(bytes& out, std::uint64_t v) {
  while (v >= 0x80) {
    out.push_back(std::uint8_t(v) | 0x80);
    v >>= 7;
  }
  out.push_back(std::uint8_t(v));
}

bool get_varint(const bytes& in, std::size_t& i, std::uint64_t& v) {
  v = 0;
  int shift = 0;
  while (i < in.size()) {
    std::uint8_t b = in[i++];
    v |= std::uint64_t(b & 0x7f) << shift;
    if (!(b & 0x80)) return true;
    shift += 7;
  }
  return false;
}

// The hello record (plaintext handshake frame): id_pk(32) | kx_pk(32) |
// sig(64) over kx_pk by the identity key.
struct hello {
  bytes id_pk, kx_pk, sig;
};

bytes encode_hello(const hello& h) {
  bytes out;
  out.insert(out.end(), h.id_pk.begin(), h.id_pk.end());
  out.insert(out.end(), h.kx_pk.begin(), h.kx_pk.end());
  out.insert(out.end(), h.sig.begin(), h.sig.end());
  return out;
}

bool decode_hello(const bytes& b, hello& h) {
  const std::size_t need = crypto_sign_PUBLICKEYBYTES + crypto_kx_PUBLICKEYBYTES +
                           crypto_sign_BYTES;
  if (b.size() != need) return false;
  auto it = b.begin();
  h.id_pk.assign(it, it + crypto_sign_PUBLICKEYBYTES);
  it += crypto_sign_PUBLICKEYBYTES;
  h.kx_pk.assign(it, it + crypto_kx_PUBLICKEYBYTES);
  it += crypto_kx_PUBLICKEYBYTES;
  h.sig.assign(it, it + crypto_sign_BYTES);
  return true;
}

// Send our secretstream header, read theirs. `send_first` orders the two
// headers so both sides agree without deadlock (initiator sends first).
struct streams {
  crypto_secretstream_xchacha20poly1305_state tx, rx;
};

bool setup_streams(int fd, const bytes& tx_key, const bytes& rx_key,
                   bool send_first, streams& s) {
  bytes hdr(crypto_secretstream_xchacha20poly1305_HEADERBYTES);
  crypto_secretstream_xchacha20poly1305_init_push(&s.tx, hdr.data(),
                                                  tx_key.data());
  bytes their_hdr;
  if (send_first) {
    if (!write_frame(fd, hdr)) return false;
    if (!read_frame(fd, their_hdr)) return false;
  } else {
    if (!read_frame(fd, their_hdr)) return false;
    if (!write_frame(fd, hdr)) return false;
  }
  if (their_hdr.size() != crypto_secretstream_xchacha20poly1305_HEADERBYTES)
    return false;
  return crypto_secretstream_xchacha20poly1305_init_pull(
             &s.rx, their_hdr.data(), rx_key.data()) == 0;
}

// The shared handshake body. `initiator` picks crypto_kx role + header order.
std::unique_ptr<channel> handshake(int fd, const identity& me,
                                   const listener::admit_fn& admit,
                                   bool initiator) {
  bytes kx_pk(crypto_kx_PUBLICKEYBYTES), kx_sk(crypto_kx_SECRETKEYBYTES);
  crypto_kx_keypair(kx_pk.data(), kx_sk.data());
  hello mine{me.public_key(), kx_pk, me.sign(kx_pk)};

  hello theirs;
  if (initiator) {
    if (!write_frame(fd, encode_hello(mine))) return nullptr;
    bytes rb;
    if (!read_frame(fd, rb) || !decode_hello(rb, theirs)) return nullptr;
  } else {
    bytes rb;
    if (!read_frame(fd, rb) || !decode_hello(rb, theirs)) return nullptr;
    if (!write_frame(fd, encode_hello(mine))) return nullptr;
  }
  // Verify the peer signed its own kx key, and that we accept its identity.
  if (!verify(theirs.id_pk, theirs.kx_pk, theirs.sig)) return nullptr;
  if (!admit(peer_key_of(theirs.id_pk))) return nullptr;

  bytes rx_key(crypto_kx_SESSIONKEYBYTES), tx_key(crypto_kx_SESSIONKEYBYTES);
  int ok = initiator
               ? crypto_kx_client_session_keys(rx_key.data(), tx_key.data(),
                                               kx_pk.data(), kx_sk.data(),
                                               theirs.kx_pk.data())
               : crypto_kx_server_session_keys(rx_key.data(), tx_key.data(),
                                               kx_pk.data(), kx_sk.data(),
                                               theirs.kx_pk.data());
  if (ok != 0) return nullptr;

  auto s = new streams();
  if (!setup_streams(fd, tx_key, rx_key, initiator, *s)) {
    delete s;
    return nullptr;
  }
  return std::make_unique<channel>(fd, s, theirs.id_pk);
}

}  // namespace

channel::channel(int fd, void* streams_ptr, bytes remote_pk)
    : fd_(fd), streams_(streams_ptr), remote_pk_(std::move(remote_pk)) {}

channel::~channel() {
  if (fd_ >= 0) ::close(fd_);
  delete static_cast<streams*>(streams_);
}

void channel::send(std::uint64_t kind, const bytes& body) {
  auto* s = static_cast<streams*>(streams_);
  bytes plain;
  put_varint(plain, kind);
  plain.insert(plain.end(), body.begin(), body.end());
  bytes cipher(plain.size() + crypto_secretstream_xchacha20poly1305_ABYTES);
  unsigned long long clen = 0;
  crypto_secretstream_xchacha20poly1305_push(&s->tx, cipher.data(), &clen,
                                             plain.data(), plain.size(),
                                             nullptr, 0, 0);
  cipher.resize(clen);
  if (!write_frame(fd_, cipher)) throw std::runtime_error("channel send failed");
}

bool channel::recv(std::uint64_t& kind, bytes& body) {
  auto* s = static_cast<streams*>(streams_);
  bytes cipher;
  if (!read_frame(fd_, cipher)) return false;
  bytes plain(cipher.size());
  unsigned long long plen = 0;
  unsigned char tag = 0;
  if (crypto_secretstream_xchacha20poly1305_pull(&s->rx, plain.data(), &plen,
                                                 &tag, cipher.data(),
                                                 cipher.size(), nullptr,
                                                 0) != 0)
    return false;
  plain.resize(plen);
  std::size_t i = 0;
  if (!get_varint(plain, i, kind)) return false;
  body.assign(plain.begin() + static_cast<long>(i), plain.end());
  return true;
}

listener::listener() {
  fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd_ < 0) throw std::runtime_error("socket() failed");
  int one = 1;
  ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = 0;
  if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0 ||
      ::listen(fd_, 16) < 0)
    throw std::runtime_error("bind/listen failed");
  socklen_t len = sizeof addr;
  ::getsockname(fd_, reinterpret_cast<sockaddr*>(&addr), &len);
  port_ = ntohs(addr.sin_port);
}

listener::~listener() {
  if (fd_ >= 0) ::close(fd_);
}

int listener::accept_raw() { return ::accept(fd_, nullptr, nullptr); }

void listener::stop() {
  if (fd_ >= 0) ::shutdown(fd_, SHUT_RDWR);
}

std::unique_ptr<channel> listener::accept(const identity& me,
                                          const admit_fn& admit) {
  int c = ::accept(fd_, nullptr, nullptr);
  if (c < 0) return nullptr;
  auto ch = handshake(c, me, admit, /*initiator=*/false);
  if (!ch) ::close(c);
  return ch;
}

std::unique_ptr<channel> dial(std::uint16_t port, const identity& me,
                              const listener::admit_fn& admit) {
  int c = ::socket(AF_INET, SOCK_STREAM, 0);
  if (c < 0) return nullptr;
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(port);
  if (::connect(c, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0) {
    ::close(c);
    return nullptr;
  }
  auto ch = handshake(c, me, admit, /*initiator=*/true);
  if (!ch) ::close(c);
  return ch;
}

bytes probe(std::uint16_t port, const bytes& payload) {
  bytes got;
  int c = ::socket(AF_INET, SOCK_STREAM, 0);
  if (c < 0) return got;
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(port);
  if (::connect(c, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0) {
    ::close(c);
    return got;
  }
  // Bound the read so a silent peer can't hang the probe: 1s recv timeout.
  timeval tv{1, 0};
  ::setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
  write_n(c, payload.data(), payload.size());
  std::uint8_t buf[512];
  auto r = ::read(c, buf, sizeof buf);
  if (r > 0) got.assign(buf, buf + r);
  ::close(c);
  return got;
}

}  // namespace syg::mesh
