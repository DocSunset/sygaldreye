# Naming — hashes, routes, addresses

Status: ratified 2026-07-02 (architecture session, Travis + Claude). This is the data
model of the whole platform; datasets.md, bootloader.md, trust.md, and rhizome.md all
lean on it.

## Two primitive name kinds

- **Hash** — a content-derived name. Names an immutable value, anywhere in the mesh,
  verifiably. What a thing *is*.
- **Local name** — a name a *container* gives one of its occupants: a node id in a
  graph, a port on a node, a ref in a store graph, a span in a text. Meaningless
  alone; meaningful one step at a time. Which *one*.

Every address is the composite: **root + route** — a root name, then a sequence of
local-name steps walked through containers.

```
b3f9a2…/nodes/osc0/out          (hash-rooted)
editor/current:nodes/osc0/out   (ref-rooted)
```

## Mutability lives in the name

Route-names absorb edits (the name stays put while the data changes; set_param
can mean mutation). Hashes deflect edits into new nodes (change the bytes
and you hold a different name). Live patches feel mutable because we address them by
route; the store feels immutable because we address it by content. Same graph
substrate, same edit ops, two naming schemes — **the store needs no special edit
vocabulary**; identity does the enforcing. Editing a hash-named node's data
or provenance links yields a new node + a rewire; testimony cannot be falsified,
only superseded.

## The liveness rule

An address is **fixed** iff it traverses only hashes and immutable containment:
it denotes one value forever and can be *normalized* — resolved to the hash of what
it reaches and memoized. An address is **live** iff it traverses at least one ref.
Fixed address wired as an input = pinned quotation; live address = subscription (a
ref move is an event that flows).

## Resolution is traversal

Each step is answered by the thing being stepped through, according to its kind: a
graph answers `nodes/`, `edges/`, port names; a wav answers channels and ranges; a
text answers spans. Every dataset kind carries a **traversal vocabulary** alongside
its render program. Resolution is demand-driven and memoizable (the astui frontier
loop — see rhizome.md). Grounding: stage 0 contains a **naive resolver** (hash →
bytes in the local object directory), the base case that lets resolution-as-graphs
avoid infinite regress — the same move as the naive executor grounding the tower.
The naive resolver stays independently invokable forever (debuggability insurance).

## Who binds what

| Mechanism | Is |
|---|---|
| **ref** | one binding: local name → hash. Its trail of past hashes is undo/history. |
| **transclusion / input link** | an input whose value is an address (fixed = quotation, live = subscription). |
| **provenance** | hashes only — testimony may not shift under you. |
| **compilation map** | route → route (app instance → execution instance); state migration re-binds along it. |
| **detachment** | a ref that stops tracking a derivation's output; fork = rebind, an ordinary loggable edit. |

Routes are keyed by local names, never positions or byte offsets — that is what
makes them edit-stable. Deterministic compilation = compilation that emits stable local
names (the lift machinery's "key by cell value, else index", stated generally).
Standalone nodes (a word, a concept) may have no containment route; routes are
*an* address, not identity.

## Sequence kinds (texts, takes, media)

Maintaining the map *position-in-current-version → hash* under heavy editing, with
versions sharing structure, is the enfilade problem (Xanadu). Implement sequence-kind
traversal with its modern descendants: ropes / persistent balanced trees /
copy-on-write B-trees (git's tree object is the degenerate directory case). Steal
Xanadu's trick outright: attach links to the *permanent identity*, derive where they
fall in any version through that version's map — bidirectional links survive editing.
Lineage note: Xanadu's permanent I-stream is this scheme's ancestor; content hashing
is its repair (eternal identity without a global address allocator).

## Formats (spelled once, boringly)

- Hashes: **CID/multihash** (self-describing hash function — the hash-migration
  problem is the one multihash was invented to solve). CID's hash+codec is nearly our
  hash+kind.
- Large blobs: **Merkle-DAG chunking** (IPFS shape) — resumable fetch, dedup between
  takes; this is datasets.md's "chunked transfer" budget item.
- Route syntax: choose late; the scheme above is the decision that matters.
