# Chapter 2 — Data, naming, kinds, types (NAM)

*The part everything else stands on. A builder implementing this chapter
delivers: the address grammar, the resolution engine, the kind registry, and
the type oracle.*

## Design

*(Encoding and grammar are pinned in ch. 14; evolution in ch. 17 —
ADR-017/025 govern.)*

**Names.** Two primitive name kinds: **hash** (CID/multihash — self-describing
hash function, chosen precisely so the hash function can migrate) and **local
name** (conferred by a container). An **address** is root + route; the root is
a hash or a ref; a route is a sequence of local names. The **liveness rule**:
fixed iff no ref is traversed; fixed addresses normalize to hashes and are
memoizable; live addresses are subscriptions.

**Resolution is traversal.** Each step is answered by the node being stepped
through, according to its kind's traversal vocabulary (a graph answers
`nodes/`, `edges/`, port names; a wav answers channels/ranges; a text answers
spans). Resolution is demand-driven and memoized. It grounds in stage 0's
naive resolver (SZ-3). Sequence kinds implement traversal with enfilade
descendants (ropes / persistent trees; git's tree object is the directory
case); links attach to hashes and positions are derived per version, so links
survive editing.

**Kinds.** One kind system for port payloads and datasets. A kind is a node:
links to decoder programs (render/traverse/serialize/validate) and conventions
(port layout, rate constraints). Kind-of-kind is `kind`; the fiat kinds
(`bytes`, `kind`, `scalar`, `audio`, `graph`, …) are sacred-ground nodes.

**Types.** A type is a promise about a link: kind × rate. Reified as a node;
declared one level up (`ports/foo/type`); **checked at edit/commit time by a
first-order program** (`connection_legal` reading two type nodes); never
consulted at tick time. Throughpoints are declarations deliberately absent —
type derived at connect time.

## Requirements

**NAM-1 (addresses).** Implement the address grammar: hash roots, ref roots,
local-name routes; parse/print round-trips exactly.
- NAM-1.1: `parse(print(a)) == a` for all addresses (property test).
- NAM-1.2: `#a11/nodes/osc0/out` resolves identically on any peer holding
  `#a11` (location independence).

**NAM-2 (liveness).** Classify every address fixed/live by traversal; fixed
addresses normalize to a hash.
- NAM-2.1: `graphs/hello-cosine:nodes/osc0/freq` is live; after
  `normalize`, `#a11/nodes/osc0/freq` is fixed and its resolution is
  memoized (second resolve performs zero I/O — observable via a counter).
- NAM-2.2: moving ref `graphs/hello-cosine` re-delivers to all live-address
  subscribers exactly once (event, never polled).

**NAM-3 (edit-stable routes).** Routes are keyed by local names only; no byte
offsets or ordinal positions above the sequence-kind substrate.
- NAM-3.1: inserting `nodes/noise0` into #b22's topology does not change the
  meaning of `nodes/osc0/freq` (contrast: an index-based scheme fails this).

**NAM-4 (one kind system).** Port payload kinds and dataset kinds resolve
against the same kind registry.
- NAM-4.1: a `scalar` on a wire and a committed `scalar` dataset carry the
  same kind hash; committing a port value requires no translation step.

**NAM-5 (type oracle).** `connection_legal(from_type, to_type)` and
`boundary_mapping(from, to)` are one shared first-order implementation used by
both parse-time validation and editor wire-drop.
- NAM-5.1: `osc0/out (audio·block) → vca0/in (audio·block)` legal, true edge.
- NAM-5.2: `lfo0/out (scalar·frame) → vca0/gain (scalar·block)` legal via
  `latch` (returned as the boundary mapping).
- NAM-5.3: `draw_call → audio` is ILLEGAL (both surfaces reject identically).
- NAM-5.4: no type-node access occurs during tick (assert via instrumentation
  in debug builds).

**NAM-6 (hash format).** CID/multihash for all hashes; Merkle-DAG chunking for
blobs above a threshold.
- NAM-6.1: re-hashing fetched data verifies it; a corrupted byte is detected.
- NAM-6.2: two takes sharing a chunk store it once (dedup observable in
  object-directory size).

**NAM-7 (sequence traversal).** Text/media kinds expose span traversal that is
stable under edits, with structural sharing between versions.
- NAM-7.1: after inserting a line at the top of a text dataset, a link
  attached to a hash-identified span still resolves to the same characters in
  the new version via the version map.

## Worked example (test seed)

The glossary's hello-cosine store fragment, plus: resolve
`graphs/hello-cosine:nodes/vca0/gain` → walk ref → #a11 → topology #b22 →
`vca0` → declaration lookup via `type → vca` → `ports/gain/type →
#t-scalar-block`. Assert: the full walk touches exactly the containers named;
normalizing after pinning the ref yields a fixed address; the type promise
compares legal against `lfo0/out` only through `latch`.
