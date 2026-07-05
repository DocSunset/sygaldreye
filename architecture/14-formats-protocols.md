# Chapter 14 — Formats and wire protocols (FMT)

*Everything that must be spelled exactly once. Governing ADRs: 007, 017, 018,
023, 028. Values marked ⚑ are pinned at first implementation and recorded
here; the shapes are decided now.*

## The canonical encoding (ADR-017)

- Structured metadata: **dag-cbor** — sorted map keys (length, then
  bytewise), definite lengths, no duplicate keys, minimal-width ints,
  canonical 64-bit floats, no NaN/Inf, links as CID (tag 42). String map
  keys only. The generated encoder is the only writer.
- Bulk payloads: **raw lane** — kind-specific encodings, hashed as-is,
  Merkle-DAG chunked above a threshold ⚑ (order 256 KiB).
- Hashes: **CIDv1** — multicodec (dag-cbor | raw ⚑) + multihash
  (blake3 or sha2-256 ⚑).
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
Fixity is per step (content-derived ⇒ fixed; conferred ⇒ fixed iff the
container is immutable; refs are the conferred-mutable steps). Signed
ref-state records: `{ peer-key, ref route, bound hash, seq, sig }` — what
SUBSCRIBE/EVENT and caches carry; verifiable, honestly stale. Reserved first
steps within graphs: `nodes` `edges` `ports` `type` `state` `topology`
`defaults` `lock`. Escaping ⚑.

## Object shapes (all dag-cbor unless noted)

- **topology** `{ nodes: {id → {type: name-into-lock}}, edges: […],
  lift_key? }`
- **defaults** `{ route → value }`
- **lock** `{ name → type CID }` (ADR-026 vocabulary lock)
- **graph** (composite) `{ kind, topology, defaults, lock, provenance }`
- **recipe provenance** `{ op, inputs: {name → CID}, determinism-class,
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

**The boot tape (ADR-028)**: the same ops in a fixed-width, CBOR-free record
format ⚑ replayable by the crown with no decoder — the movement's only
input format. A graph ≡ its building ops, so tape ↔ topology conversion is
mechanical.

## Wire protocol (peer-level conformance surface)

All messages authenticated under pairing keys (MSH-2); payloads dag-cbor.

| message | carries |
|---|---|
| PAIR | one-time ceremony: key exchange + acceptance record |
| HELLO / ADVERTISE | the three lists: run (type CIDs), serve (dataset CIDs / roots), subscribe |
| SUBSCRIBE / EVENT | live-address subscription; ref-move and value events |
| OPS | attributed edit ops toward an instance's arbiter (ADR-023) |
| FETCH / CHUNK | content-addressed get; resumable Merkle chunks |
| QUERY | a query graph (or its CID) + arguments; answers are datasets |
| PLACE / REFUSE | compilation placement request; typed refusal (MSH-3.1) |
| FAULT | fault records crossing peers (just EVENTs on fault outputs) |

Transport: WebSocket-capable framing (browser constraint), one duplex
channel per peer pair, message kinds multiplexed ⚑.

## Compilation map

`{ app-route → execution-route }`, dag-cbor, emitted with every compile
(CMP-2), consumed by migration and projection editing. Deterministic: same
compile inputs → byte-identical map (exact-class derivation).

## Requirements

**FMT-1.** Encoder conformance: a corpus of value/bytes pairs (grown per
kind automatically — ADR-017) round-trips in every host language binding.
**FMT-2.** Address grammar: parse∘print identity property test (NAM-1.1)
against a generated corpus including all reserved steps and escapes.
**FMT-3.** Boot tape: tape → crown replay → serialize yields the same
topology hash as the tape's source graph.
**FMT-4.** Wire golden transcripts: a recorded two-peer session (pair,
advertise, subscribe, ops, fetch, place) replays byte-exact against a
candidate implementation (the peer-level conformance backbone, ch. 17).
**FMT-5.** Every ⚑ pin, once chosen, is recorded in this chapter in the
same commit as its first implementation.
