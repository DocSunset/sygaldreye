# Chapter 3 — Store, commit, availability (STO)

*A builder implementing this chapter delivers: the object directory machinery,
the store-graph face, the two commit paths, refs with history, provide/
compatible advertisement, and fetch-by-hash.*

## Design

The store is **a graph of dataset nodes** — the store package's vocabulary is
its face; the on-disk object directory (data_dir on Linux/Quest, IndexedDB in
browser) is its machinery (the dac relationship). It never ticks and is
demand-materialized. Access is lexical: a graph reaches a store by wiring to
it; stores are plural, nested, scoped (per project, document, collaboration).
The store is NOT stage 0 — the embedded boot graph boots a peer before or
without one; stage 0 carries only the naive resolver.

No special edit vocabulary: identity enforces immutability. Editing a
hash-named node's data or provenance yields a new node + rewire; refs are the
mutable layer, and a ref move is an ordinary param write whose event flows.

**Two commit paths.** Derivation (recipe; memoizable; evictable) and capture
(testimony: peer public key, wiring, per-peer wall-clock as description only —
nothing load-bearing reads timestamps; causal order comes from ref hash
chains). A graph is a **composite node** (topology + defaults); presets are
alternate defaults nodes.

**Availability: provide/compatible.** A peer *provides* a dataset (kept +
advertised = the promise it can be fetched here) or is merely *compatible*.
Durability = someone provides it. Caches evict non-provided copies freely, no
coordination — derived data re-derives; a capture is **provisional until
provided** and the recorder must surface that. "Archive" = a peer whose policy
is provide-everything (the household mesh should run one). Ref history:
forever on provide-everything peers, bounded on caches. Providing a dataset
does not obligate providing its provenance closure; "fully-provided closure"
is a queryable mesh-health property. Human retention: forgetting is a
supported decision — all providers unpin, caches converge.

**Back-links.** Provenance points upstream; each store indexes the downstream
inverse as records are committed; queries fan out over the mesh (honest =
bounded by trust domain).

## Requirements

**STO-1 (object machinery).** Content-addressed put/get over the object
directory; verification is re-hashing.
- STO-1.1: put(bytes) → hash; get(hash) → identical bytes; get of unknown
  hash is a clean miss (triggering fetch, STO-6).

**STO-2 (store-graph face).** get = wiring, put = graph edit, ref = named
node, query = traversal; no ambient store API anywhere in node code.
- STO-2.1: hello-cosine's editor session reads #b22 by wiring to the store
  graph; grep confirms no global store singleton is reachable from node
  implementations.

**STO-3 (commit paths).** Committing records provenance as recipe (derive)
or testimony (capture); a stream cannot be committed directly.
- STO-3.1: compiling #a11 yields an execution-graph dataset whose provenance
  = (compile, inputs: #a11, engine-hash); re-compiling is a memo hit (zero
  passes run — counter-observable).
- STO-3.2: recording 2 s of dac output yields a wav dataset whose testimony
  carries peer key + wiring route `nodes/dac0/in`; attempting to "commit the
  adc stream" without a recorder node is a type error.

**STO-4 (refs and undo).** Refs bind local name → hash; every rebind appends
to the trail; undo = rebind to trail predecessor.
- STO-4.1: edit hello-cosine (freq 220→330), commit, undo: ref points at
  #a11 again; both versions remain fetchable.

**STO-5 (provide/compatible).** Peers advertise provided datasets alongside
provided node types; compatible-only copies are evictable.
- STO-5.1: Quest holds #a11 unprovided; after cache pressure, #a11 is gone
  locally but re-fetchable from the Linux peer (which provides it).
- STO-5.2: a fresh capture on Quest reports `provided-by: nobody` until the
  archive replicates it, then flips — the flag is a query, not stored state.

**STO-6 (fetch).** Content-addressed fetch by hash from any providing peer;
idempotent, cacheable, chunked for large blobs; resumable.
- STO-6.1: interrupting a 100 MB take transfer at 50% and retrying moves only
  the missing chunks.

**STO-7 (composite graphs).** Topology and defaults are separate datasets;
single-file JSON remains the interchange form.
- STO-7.1: two presets of hello-cosine share topology hash #b22 (observable:
  one topology object, two defaults objects).
- STO-7.2: deleting osc0 from the topology surfaces a conflict for the
  orphaned default `osc0/freq` (rebase semantics), not silence.

**STO-8 (retention & forgetting).** Unpin-everywhere converges: after all
providers drop a dataset and caches cycle, the hash is unfetchable in-mesh.
- STO-8.1: integration test across three peers confirms convergence and that
  provenance records referencing the forgotten hash still parse (dangling is
  legal, dishonest is not).

**STO-9 (back-link index).** Each store indexes provenance's downstream
inverse at commit time; `query(lineage: #a11)` lists the execution graph and
the take.
- STO-9.1: the index is itself derived data — deleting and re-deriving it
  yields an identical index (hash-equal).

## Worked example (test seed)

Three-peer scenario: Linux (provide-everything), Quest (cache), browser
(cache). Author hello-cosine on Linux; run on Quest (fetch by hash); record a
take on Quest (capture, provisional → provided after sync); edit freq; undo;
evict on Quest; re-fetch; forget the take deliberately. Each step's assertions
are the STO acceptance criteria above, in order.
