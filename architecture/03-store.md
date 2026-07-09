# Chapter 3 — Store, commit, availability (STO)

*A builder implementing this chapter delivers: the object directory machinery,
the store-graph face, the two commit paths, refs with history, provide/
compatible advertisement, and fetch-by-hash.*

## Design

*(Object encodings: ch. 14 / ADR-017. History is the op tree: ADR-018.
Availability naming: provide = pin, per the lexicon.)*

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

**sto.object_machinery** Content-addressed put/get over the object
directory; verification is re-hashing.
- sto.object_machinery.put_get_roundtrip: put(bytes) to hash; get(hash) to identical bytes; get of unknown
  hash is a clean miss (triggering fetch, sto.fetch).

**sto.store_graph_face** get = wiring, put = graph edit, ref = named
node, query = traversal; no ambient store API anywhere in node code.
- sto.store_graph_face.no_ambient_store: hello-cosine's editor session reads #b22 by wiring to the store
  graph; grep confirms no global store singleton is reachable from node
  implementations.

**sto.commit_paths** Committing records provenance as recipe (derive)
or testimony (capture); a stream cannot be committed directly.
- sto.commit_paths.compile_recipe_memo: compiling #a11 yields an execution-graph dataset whose provenance
  = (compile, inputs: #a11, engine-hash); re-compiling is a memo hit (zero
  passes run — counter-observable).
- sto.commit_paths.capture_testimony: recording 2 s of dac output yields a wav dataset whose testimony
  carries peer key + wiring route `nodes/dac0/in`; attempting to "commit the
  adc stream" without a recorder node is refused at edit time.

**sto.refs_and_undo** Refs bind local name to hash; every rebind appends
to the trail; undo = rebind to trail predecessor.
- sto.refs_and_undo.undo_ref_trail: edit hello-cosine (freq 220 to 330), commit, undo: ref points at
  #a11 again; both versions remain fetchable.

**sto.provide_compatible** Peers advertise provided datasets alongside
provided node types; compatible-only copies are evictable.
- sto.provide_compatible.evict_refetch: Quest holds #a11 unprovided; after cache pressure, #a11 is gone
  locally but re-fetchable from the Linux peer (which provides it).
- sto.provide_compatible.provided_is_query: a fresh capture on Quest reports `provided-by: nobody` until the
  archive replicates it, then flips — the flag is a query, not stored state.

**sto.fetch** Content-addressed fetch by hash from any providing peer;
idempotent, cacheable, chunked for large blobs; resumable.
- sto.fetch.resumable_fetch: interrupting a 100 MB take transfer at 50% and retrying moves only
  the missing chunks.

**sto.composite_graphs** Topology and defaults are separate datasets;
single-file JSON remains the interchange form.
- sto.composite_graphs.shared_topology: two presets of hello-cosine share topology hash #b22 (observable:
  one topology object, two defaults objects).
- sto.composite_graphs.orphan_conflict: deleting osc0 from the topology surfaces a conflict for the
  orphaned default `osc0/freq` (rebase semantics), not silence.

**sto.retention_and_forgetting** Unpin-everywhere converges: after all
providers drop a dataset and caches cycle, the hash is unfetchable in-mesh.
- sto.retention_and_forgetting.forgetting_converges: integration test across three peers confirms convergence and that
  provenance records referencing the forgotten hash still parse (dangling is
  legal, dishonest is not).

**sto.back_link_index** Each store indexes provenance's downstream
inverse at commit time; `query(lineage: #a11)` lists the execution graph and
the take.
- sto.back_link_index.index_is_derived: the index is itself derived data — deleting and re-deriving it
  yields an identical index (hash-equal).

## Worked example (test seed)

Three-peer scenario: Linux (provide-everything), Quest (cache), browser
(cache). Author hello-cosine on Linux; run on Quest (fetch by hash); record a
take on Quest (capture, provisional to provided after sync); edit freq; undo;
evict on Quest; re-fetch; forget the take deliberately. Each step's assertions
are the STO acceptance criteria above, in order.
