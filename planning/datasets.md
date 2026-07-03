# Datasets — store, commit paths, availability

Status: ratified 2026-07-02 (architecture session, Travis + Claude), superseding the
2026-07-02 draft. Companion to planning/bootloader.md (datasets-and-derivations
principle). Naming/addressing: planning/naming.md. Trust: planning/trust.md.

## Object model

Immutable content-addressed objects + mutable refs — but see naming.md: mutability
is a property of the *name*, not the thing, and the store needs no special edit
vocabulary. A dataset is bytes + kind + provenance header, identified by hash
(CID/multihash; Merkle-DAG chunking for large blobs — naming.md "Formats").
Provenance-or-fork holds by construction. Derivation memoization = lookup keyed by
input hashes. Git/Nix object model, IPFS formats.

**A dataset is a node.** Outputs are its contents (finished values; a dataset node
never ticks — reading is reference resolution). Inputs are links to other nodes'
contents. The provenance header of a derived dataset ≈ its kind + its input links
with hashes resolved at commit time (a Nix .drv: builder + input hashes).

**A graph is a composite node**: a *topology* node + a *defaults* node,
linked. Presets = alternate defaults nodes. Defaults are keyed by routes into
the topology; an orphaning topology edit surfaces as a rebase conflict. Today's
single graph JSON survives as the interchange form (a composite serializes inline);
the split is logical, at the store level.

## Two commit paths

Dataset-ness is the nature of an act, not an annotation. A stream cannot promise
immutability; a written-down take can.

- **Derivation** — inputs are dataset hashes. Provenance is a *recipe*:
  re-derivable, memoizable, safe to evict (worst case, re-derive).
- **Capture** — the input was the world: a mic, a hand, a moment. Provenance is
  *testimony*, not a recipe — which peer (a public key — trust.md), what wiring,
  per-peer wall-clock as *description only* (the mesh has no one clock; nothing in
  identity, ordering, or conflict resolution consults timestamps — causal order
  comes from ref hash-chains). The take is original and irreplaceable.

Every derivation chain bottoms out in captures and fiat-named nodes — the
sacred ground of the dataset graph (rhizome.md).

## The store is a graph

Not a per-peer service with ambient authority — a **graph of dataset nodes**,
reached only by wiring to it (lexical, capability-style; stores can be plural,
nested, scoped per project/document/collaboration). The old store-node vocabulary
dissolves: get = wiring/transclusion; put = a graph edit (add node, move ref);
ref = a named node whose moves are events that flow; query = traversal (back-links
= provenance's downstream inverse, indexed per store as records are committed).
Remote store access = remote graph access (existing remote-node/ws machinery).
Undo unifies: ref history and graph-edit history are one mechanism.

The store graph never ticks and is demand-materialized (the tower's laziness rule).
The on-disk object directory (data_dir; IndexedDB in browser) is the store
package's *machinery*; the graph is its face — the dac relationship. The store is
**not stage 0** (the embedded boot graph boots a peer before/without a store), but
stage 0 carries the **naive resolver** (hash → bytes locally) that grounds
resolution — naming.md.

## Availability: provide / compatible

No distributed GC, no consensus, no special archive concept. Reusing the
capability model (a peer advertises the nodes it supports):

- A peer that holds a copy and decides to keep it **provides** that dataset node —
  the same speech act as "I provide dac": a promise it can be fetched here.
- A peer that can make sense of a dataset (has its kind's vocabulary) is
  **compatible** — it may hold a copy and promise nothing.

Consequences:

- Durability = "someone provides it" — answerable by the existing discovery/query
  fan-out. A capture is **provisional until some peer provides it** (a take
  recorded on the Quest with the archive unreachable is at risk; the recorder
  should surface un-provided status, cleared on replication).
- GC per peer: evict non-provided copies freely, no coordination. Derived data is
  always reconstructible; that's what the derive/capture split buys.
- "Archive" is deployment policy, not architecture: a peer configured to provide
  everything it sees (the household mesh should have one, or captures die with
  headset storage). Ref history: forever on provide-everything peers, bounded on
  caches. Multi-archive redundancy = two peers, same policy.
- Providing a dataset does not obligate providing its provenance closure;
  "fully-provided closure" is a queryable mesh-health property and an optional
  providing policy.
- **Human retention**: some captures are recordings of people. Forgetting is a
  supported decision, not an accident: all providers unpin (stop providing),
  caches converge by eviction. Honest within one trust domain.

## Scope

Port traffic is not datasets; a value becomes a dataset when committed (one of the
two paths above). Kinds: graphs (topology, defaults, engine, execution), derived
artifacts (frozen .so, analyses, models), media (wavs, fonts, textures),
provenance records, documents (rhizome.md). Not built: distributed consensus,
CRDTs, quotas, encryption at rest, per-dataset ACLs. Single-writer-per-ref;
collaboration machinery waits for collaborators.
