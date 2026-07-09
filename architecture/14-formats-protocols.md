# Chapter 14 — Formats and wire protocols (FMT)

*Everything that must be spelled exactly once. Governing ADRs: 007, 017, 018,
023, 028, 031. All pins were chosen and recorded 2026-07-05 (fmt.pins_frozen): boring on
purpose, frozen on purpose.*

## The canonical encoding (ADR-017)

- Structured metadata: **dag-cbor** — sorted map keys (length, then
  bytewise), definite lengths, no duplicate keys, minimal-width ints,
  canonical 64-bit floats, no NaN/Inf, links as CID (tag 42). String map
  keys only. The generated encoder is the only writer.
- Bulk payloads: **raw lane** — kind-specific encodings, hashed as-is,
  Merkle-DAG chunked above 256 KiB (pinned): fixed-size 256 KiB chunks, listed
  by a dag-cbor index node of links — simple, resumable, dedup-friendly.
- Hashes: **CIDv1** (pinned) — multibase base32; multicodec dag-cbor (0x71)
  and raw (0x55); multihash **blake3-256** (0x1e).
- **The header is the one non-self-describing layer**: the CID prefix
  itself, compiled into every reader. Frozen by design; succession of this
  spec is the only flag-day event the system admits, and multiformats exist
  to make even that additive.

## The address grammar (ADR-007, ADR-029)

Semantically an address is a route walked from the resolver's environment
node; the spellings below are serialization sugar for common first steps:

```
address  := refname [ ":" route ]        (lexical: first step into the wired store env)
          | cid [ "/" route ]            (first step into the object store; verified)
          | "#" peerkey "/" route        (self-certifying: first step into the peer table)
route    := step ( "/" step )*
step     := localname                    (container-conferred; no positions, no offsets)
```
Fixity is per step (content-derived implies fixed; conferred implies fixed if and only if the
container is immutable; refs are the conferred-mutable steps). Signed
ref-state records: `{ peer-key, ref route, bound hash, seq, sig }` — what
SUBSCRIBE/EVENT and caches carry; verifiable, honestly stale. Reserved first
steps within graphs: `nodes` `edges` `ports` `type` `state` `topology`
`defaults` `lock`. Escaping (pinned): percent-encode `%`, `/`, `:`, `#`, and
whitespace inside a local name; nothing else is ever escaped.

## Object shapes (all dag-cbor unless noted)

- **topology** `{ nodes: {id to {type: name-into-lock}}, edges: [...],
  lift_key? }`
- **defaults** `{ route to value }`
- **lock** `{ name to type CID }` (ADR-026 vocabulary lock)
- **graph** (composite) `{ kind, topology, defaults, lock, provenance }`
- **recipe provenance** `{ op, inputs: {name to CID}, determinism-class,
  output(s), signer }` (ADR-021: approximate recipes may list multiple
  outputs)
- **testimony** `{ peer-key, wiring-route, wallclock (descriptive only),
  free-form }`
- **node type / kind / type node**: as ch. 2 and ch. 13 — declarations one
  level up; contract hash recorded.
- **fault record** (ADR-016): `{ route, class, region, executor-clock,
  detail }`.

## Edit ops (ADR-018, ADR-023)

`{ op: add_node|remove_node|add_edge|remove_edge|set_param|replace_graph,
args (routes + values), prev (inverse payload), pre (preconditions: routes
that must exist/absent), author: peer-key, gesture: id }` — coalescable
within a gesture bracket; the log is a tree (parent links); commit folds a
range.

**The boot tape (ADR-028)**: the same ops in a CBOR-free record format
(pinned): newline-delimited ASCII records, space-separated fields,
percent-escaped as above — `NODE id kind`, `LINK from-route to-route`,
`SET route value` — replayable by the crown with no decoder — the movement's only
input format. A graph is equivalent to its building ops, so tape and back to topology conversion is
mechanical.

## Wire protocol (peer-level conformance surface)

All messages authenticated under pairing keys (msh.authenticated_transport); payloads dag-cbor.

| message | carries |
|---|---|
| PAIR | one-time ceremony: key exchange + acceptance record |
| HELLO / ADVERTISE | the three lists: run (type CIDs), serve (dataset CIDs / roots), subscribe |
| SUBSCRIBE / EVENT | live-address subscription; ref-move and value events |
| OPS | attributed edit ops toward an instance's arbiter (ADR-023) |
| FETCH / CHUNK | content-addressed get; resumable Merkle chunks |
| QUERY | a query graph (or its CID) + arguments; answers are datasets |
| PLACE / REFUSE | compilation placement request; typed refusal (msh.three_lists.typed_refusal) |
| FAULT | fault records crossing peers (just EVENTs on fault outputs) |

Transport: WebSocket-capable framing (browser constraint), one duplex
channel per peer pair; multiplexing (pinned): each message is one varint
message-kind (numbered 1-8 in table order above) followed by a dag-cbor body.

**Crypto suite (pinned — ADR-035).** Identity, signatures, capture
testimony, and provenance signing: **Ed25519** (a peer-key is the 32-byte
public key, text-encoded exactly as the `#peerkey` address step —
multibase base32). Pairing/session handshake: exchange identity keys +
ephemeral **X25519** key-exchange keys (`crypto_kx`), each side signing its
ephemeral key with its identity key; a peer accepts iff the signature
verifies and the remote identity key is in its accepted-keys set.
Channel confidentiality+integrity: **XChaCha20-Poly1305**
(`crypto_secretstream`) keyed by the handshake — every framed message is one
secretstream message. Revocation drops the key from the accepted set and
severs live sessions. Unpaired ⇒ no service surface (msh.authenticated_transport). Boring on
purpose; changing a primitive is a succession, never an edit (fmt.pins_frozen).

## Compilation map

`{ app-route to execution-route }`, dag-cbor, emitted with every compile
(cmp.determinism_and_map), consumed by migration and projection editing. Deterministic: same
compile inputs to byte-identical map (exact-class derivation).

## Requirements

**fmt.encoder_conformance** Encoder conformance: a corpus of value/bytes pairs (grown per
kind automatically — ADR-017) round-trips in every host language binding.
**fmt.address_grammar** Address grammar: parse-after-print identity property test (nam.addresses.parse_print_roundtrip)
against a generated corpus including all reserved steps and escapes.
**fmt.boot_tape** Boot tape: tape to crown replay to serialize yields the same
topology hash as the tape's source graph.
**fmt.wire_transcripts** Wire golden transcripts: a recorded two-peer session (pair,
advertise, subscribe, ops, fetch, place) replays byte-exact against a
candidate implementation (the peer-level conformance backbone, ch. 17).
**fmt.pins_frozen** Every pin above is FROZEN as of 2026-07-05; changing one is a
succession of this spec (ADR-025), never an edit.
