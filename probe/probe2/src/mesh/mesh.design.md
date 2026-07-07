# mesh (package)

Peer identity, authenticated encrypted transport, and the wire protocol
(ch. 8, ch. 14; ADR-035). Peer-level conformance is born here: a `syg mesh`
session boots N in-process peers that talk over REAL loopback sockets, so
pairing, revocation, advertisement, fetch, placement, and the port-scan
refusal are exercised the way a mesh citizen exercises them, not modeled.

## Components

- **identity** — Ed25519 keypair (libsodium). A peer-key IS the public key,
  text-encoded exactly as a CID (`#peerkey` address step, multibase base32 —
  the one text codec, shared with `formats/cid`). Seeded keys make tests
  exact-class. Clause: machinery (crypto is a maturity import, ADR-033).
- **link** — the one place that speaks the wire. A TCP loopback `listener`,
  `dial`, and a framed `channel`. The handshake exchanges identity + ephemeral
  X25519 keys (`crypto_kx`), each side signing its ephemeral key; a peer
  admits the other iff the signature verifies AND a live `admit(peer_key)`
  predicate approves it (so revocation mid-session takes effect at once). The
  channel is XChaCha20-Poly1305 `crypto_secretstream`; every message is one
  varint wire-kind + dag-cbor body. `probe()` is the raw unauthenticated
  scanner surface (MSH-2.1): a conformant peer answers it with zero bytes.

## Ports (session driver — `src/syg/mesh_session.cpp`)

- Inputs: a scripted op list (peers + ops) on stdin.
- Outputs: per-op results on stdout (JSON), including typed refusals.
- Sources: each peer's store, advertisement lists, and pairing table.
- Destinations: the wire (loopback sockets between in-process peers).
- Temporal couplings: a peer's serve loop must be running before another
  peer dials it (all serve threads start at session boot); revocation
  affects only handshakes that begin after it commits.
- Seams: **discovery** is an abstract provider — the household map here is
  the static peer list (MSH-7); an mDNS beacon would populate the same map
  without touching any semantics. **Sharing** is per store graph (MSH-8):
  the default store is "shared with every paired key"; a `share-store` op
  adds one scoped to a key subset.

## Allowed dependencies

`formats/cid`, `formats/dagcbor`, `store`, libsodium. The wire dispatch
reuses the store/advertisement organs; it never reimplements their logic.

## Owning package

mesh.
