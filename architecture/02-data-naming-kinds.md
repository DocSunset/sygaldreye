# Chapter 2 — Data, naming, kinds (NAM)

*The part everything else stands on. A builder implementing this chapter
delivers: the address grammar, the resolution engine, the kind registry, and
the promise oracle.*

## Design

*(Encoding and grammar are pinned in ch. 14; evolution in ch. 17 —
ADR-017 and 025 govern.)*

**Names.** Two primitive name kinds: **hash** (CID/multihash — self-describing
hash function, chosen precisely so the hash function can migrate) and **local
name** (conferred by a container). An **address is a route walked from
HERE** (ADR-029): resolution begins at the resolver's environment node — its
wired stores, object store, peer table, petnames (astui's ground,
per-resolver, never global). What looks like a root is the first step,
answered by an environment container; `root:route` is spelling sugar. The
**liveness rule**, per step: fixed if and only if the step's name is content-derived
(hash — pinned by verification) or conferred by immutable containment; an
address is fixed if and only if every step is (normalizable, memoizable), live if any
step crosses a ref (a subscription).

**Finding first steps.** Unqualified ref-names resolve lexically against the
wired store environment, like a relative path against a cwd — there is no
global ref registry. The only mesh-wide namespace is the key space:
`#peer-key/...` steps are self-certifying (the name carries its verification
material); ref-states travel signed; each peer is sole authority for its own
refs (single-writer); discovery is advertisement, never registration;
petnames humanize keys in a private local store graph.

**Resolution is traversal.** Each step is answered by the node being stepped
through, according to its kind's traversal vocabulary (a graph answers
`nodes/`, `edges/`, port names; a wav answers channels/ranges; a text answers
spans). Resolution is demand-driven and memoized. It grounds in stage 0's
naive resolver (sz.naive_resolver). Sequence kinds implement traversal with enfilade
descendants (ropes / persistent trees; git's tree object is the directory
case); links attach to hashes and positions are derived per version, so links
survive editing.

**Kinds.** One kind system for port payloads and datasets. A kind is a node:
links to decoder programs (render/traverse/serialize/validate) and conventions
(port layout, rate constraints). Kind-of-kind is `kind`; the fiat kinds
(`bytes`, `kind`, `scalar`, `audio`, `graph`, ...) are sacred-ground nodes.

**The kind authoring surface (ADR-017 + ADR-030, made explicit 2026-07-05).**
For struct-shaped kinds, the canonical definition surface is a C/C++ schema
struct reflected via PFR (static reflection later): **fields are links** —
each named member is a named link in the kind's convention, so the struct IS
the one-level-up declaration written in the machine's language. The generator
derives everything from it (in-motion layout, canonical codec, JSON
projection, descriptor), and the committed descriptor IS the kind node
(cmp.engine_is_realized.honest_lock) — so a C++ type and a kind correspond **by generated declaration,
never by convention or folklore**. A node type's endpoints struct is this
same surface for a kind that carries behavior (ADR-030); bulk raw-lane kinds
(audio, wav, mesh payloads) author decoders instead of schemas (ADR-017's two
lanes). A hand-maintained kind table is the descriptor-drift bug this design
exists to kill (aut.generated_descriptors).

**Port promises (ADR-030 — "type" dissolved).** A port declaration attaches
its promises directly as links: `ports/foo/kind` and `ports/foo/discipline`
(clock constraints ride the kind node — audio pins block). **Checked at
edit/commit time by a first-order program** (`connection_legal` reading the
(kind, discipline) pairs); never consulted at tick time. Throughpoints are
declarations deliberately absent — promises derived at connect time. And one
mechanism classifies everything: a node type is a kind that carries behavior;
an instance's `type` link is its kind link.

## Requirements

**nam.addresses** Implement the address grammar: first steps of all
three spellings (lexical ref-name, cid, peer-key), local-name routes;
parse/print round-trips exactly; resolution starts at the environment node.
- nam.addresses.parse_print_roundtrip: `parse(print(a)) == a` for all addresses (property test).
- nam.addresses.location_independent: `#a11/nodes/osc0/out` resolves identically on any peer holding
  `#a11` (location independence).

**nam.liveness** Classify every address fixed/live by traversal; fixed
addresses normalize to a hash.
- nam.liveness.live_fixed_memo: `graphs/hello-cosine:nodes/osc0/freq` is live; after
  `normalize`, `#a11/nodes/osc0/freq` is fixed and its resolution is
  memoized (second resolve performs zero I/O — observable via a counter).
- nam.liveness.ref_move_delivers_once: moving ref `graphs/hello-cosine` re-delivers to all live-address
  subscribers exactly once (event, never polled).

**nam.edit_stable_routes** Routes are keyed by local names only; no byte
offsets or ordinal positions above the sequence-kind substrate.
- nam.edit_stable_routes.insertion_stable: inserting `nodes/noise0` into #b22's topology does not change the
  meaning of `nodes/osc0/freq` (contrast: an index-based scheme fails this).

**nam.one_kind_system** Port payload kinds and dataset kinds resolve
against the same kind registry.
- nam.one_kind_system.one_kind_hash: a `scalar` on a wire and a committed `scalar` dataset carry the
  same kind hash; committing a port value requires no translation step.

**nam.promise_oracle** `connection_legal(from, to)` over (kind,
discipline) pairs and
`boundary_mapping(from, to)` are one shared first-order implementation used by
both parse-time validation and editor wire-drop.
- nam.promise_oracle.true_edge_legal: `osc0/out (audio, block) to vca0/in (audio, block)` legal, true edge.
- nam.promise_oracle.latch_boundary: `lfo0/out (scalar, frame) to vca0/gain (scalar, block)` legal via
  `latch` (returned as the boundary mapping).
- nam.promise_oracle.kind_mismatch_illegal: `draw_call to audio` is ILLEGAL (both surfaces reject identically).
- nam.promise_oracle.no_lookup_at_tick: no kind/promise lookup occurs during tick (assert via instrumentation
  in debug builds).

**nam.hash_format** CID/multihash for all hashes; Merkle-DAG chunking for
blobs above a threshold.
- nam.hash_format.rehash_verifies: re-hashing fetched data verifies it; a corrupted byte is detected.
- nam.hash_format.chunk_dedup: two takes sharing a chunk store it once (dedup observable in
  object-directory size).

**nam.sequence_traversal** Text/media kinds expose span traversal that is
stable under edits, with structural sharing between versions.
- nam.sequence_traversal.span_survives_edit: after inserting a line at the top of a text dataset, a link
  attached to a hash-identified span still resolves to the same characters in
  the new version via the version map.

## Worked example (test seed)

The glossary's hello-cosine store fragment, plus: resolve
`graphs/hello-cosine:nodes/vca0/gain` to walk ref to #a11 to topology #b22  to 
`vca0` to declaration lookup via `type to vca` to `ports/gain/kind to scalar`,
`ports/gain/discipline to stream`. Assert: the full walk touches exactly the
containers named;
normalizing after pinning the ref yields a fixed address; the promise pair
compares legal against `lfo0/out` only through `latch`.
