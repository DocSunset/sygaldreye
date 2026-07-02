# Datasets — store, visibility, access

Status: design draft, 2026-07-02, from alignment discussion. Companion to
planning/bootloader.md (datasets-and-derivations principle). Open: Q1–Q3 below.

## Object model: content addressing

Immutable content-addressed objects + a mutable name layer. A dataset is
bytes + kind + provenance header, identified by hash. A **ref** (`editor/current`,
`synths/chime`) is a mutable name → hash. Edits write a new object and move the
ref; nothing mutates in place. This enforces the provenance-or-fork law by
construction: fork = copy a ref; detachment = a ref that stops tracking a
derivation's output; undo/history = the ref's hash trail (cf. undo_node);
derivation memoization = lookup keyed by input hashes. Git/Nix object model.

## Scope

Port traffic is not datasets. A value becomes a dataset when committed to the
store — explicitly by a node, or implicitly by derivation recording. Kinds (all
exist as loose files today): graphs (app/engine/execution definitions), derived
artifacts (frozen .so, analyses, models), media (wavs, fonts, textures —
assets/), provenance records.

Not built: distributed consensus, CRDTs, quotas, encryption at rest, per-dataset
ACLs. Single-writer-per-ref discipline; collaboration machinery waits for
collaborators.

## Placement: symmetric protocol, asymmetric policy

Every peer has a local store (directory under data_dir on Linux/Quest; IndexedDB
in browser). Replication is by hash, on demand — location-independent identity,
verification is re-hashing. Per-peer pinning/retention policy: Linux peer pins
everything (the archive); Quest and browser are evicting caches. Nix-substituter
shape. GC roots = named refs + provenance closure of pinned artifacts.

The store is **not stage 0** — it is a capability package (store nodes +
machinery). The embedded boot graph is what lets a peer boot before or without a
store.

## Visibility: the mesh is a trust domain and a security boundary

Capability-driven placement means graphs are remote code execution by design, so
mesh membership is a security boundary. Model: one trust domain, pre-shared key.
mDNS advertises presence; store access and region placement require membership.
Inside the mesh everything is visible; namespaces organize, they do not secure.
Identity/ACLs deferred to actual multi-user; content addressing makes them
retrofittable (gate refs and fetch, not the data model).

Today's reality to retire: the HTTP control surface is unauthenticated on the LAN.

## Access: nodes

`store_put`, `store_get` (hash), `store_ref` (name; subscribing — a ref move is
an event that flows), `store_query` (kind, lineage). Transport: per-peer HTTP/WS;
content-addressed GET is idempotent and cacheable. Discovery generalizes
/palette: peers answer queries about holdings as they do about capabilities.
Budget item: chunked transfer for large blobs over the bridge.

## Open questions

1. Confirm single-trust-domain + PSK for the foreseeable horizon (the root of any
   later multi-user story).
2. Hash function + object header format — trivial now, migration-painful later.
3. Ref history retention: forever on the archive peer, bounded on caches
   (recommended, consistent with pinning asymmetry)?
