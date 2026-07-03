# Rhizome — the document layer (subsumed 2026-07-02)

Sygaldreye subsumes the Rhizome project. `~/agents/projects/rhizome` (formerly
astui) is an **abandoned design probe**; this file migrates its durable context.
Everything below rests on naming.md, datasets.md, and trust.md.

## The dream (Travis, 2026-06-24)

A natively **all-media, LLM-era hypermedium** — the web Xanadu wanted, built for
media and AI from the ground up. Sygaldreye is the computing substrate; Rhizome is
the document/medium that lives on it. Lineage, one feature each: Xanadu
(transclusion + true bidirectional links), IPFS (content-addressed immutable
contents), git (versioning/history), BitTorrent/libmapper (P2P meshing).

The model: **every entity is a bag of links** — to other entities, to media
contents, or it *is* contents — and **every entity carries a proposed program** to
render, serialize, or save it. Representation is computed, not fixed.

## The unification

**A rhizome entity is a dataset is a node** (ratified 2026-07-02; per the
2026-07-03 lexicon, "entity" is subsumed — say node). Consequences:

- Transclusion = an input link holding an address (fixed = quotation, live =
  subscription — naming.md). A document is a graph; the editor is a rhizome
  browser.
- The "proposed program to render" = the node's type / an engine graph
  whose input kind is "document". *Proposed* means declinable: it runs in the
  reader's advertised sandbox vocabulary, and a compatible peer may render the
  kind its own way (trust.md).
- Every open thread on the original dream card is answered elsewhere now:
  immutable + mutable history + live links → the liveness rule (naming.md);
  back-links over P2P → provenance's downstream inverse, indexed per store,
  queried across the mesh (bounded by trust domain — honest, not global truth;
  DHT-style discovery only at world scale); live vs archival medium → streaming
  vs committed, the two commit paths (datasets.md).

## The probe's durable results (astui)

astui explored constructing understanding of a codebase as a navigable map of its
problem–solution structure. What survives as design input:

- **Lines and edges, uniformly**: one element; point vs line is perspective, not
  structure; an edge is itself a line and can be an endpoint of another edge.
  This is the node–edge uniformity the store graph inherits.
- **The ground**: walks bottom out, rarely and deliberately, at sacred primitive
  words grounded by fiat. Generalized: **captures and fiat-named nodes are the
  sacred ground of the dataset graph** — every derivation chain bottoms out in
  testimony (datasets.md).
- **Navigation model**: here (always a single line, no reified position), path
  (breadcrumb history, not a menu), frontier (what you can reach from here,
  recomputed each step), mark (the authoring primitive; the marked subgraph + hand-
  authored connections = the durable map). This is the store browser's UX.
- **Demand-driven, incremental, memoized relation graph, almost nothing stored,
  repairing itself as sources change** — this is resolution + derivation repair,
  i.e. spreadsheet recalc; the store graph's read side.
- **The keystone question, now answered**: "how a line names itself" — hash for
  the node, routes for instances, mutability lives in the name (naming.md).
  Standalone lines (the namespace `astui`, the word `C++`) are nodes with no
  containment route — legal; routes are an address, not identity.
- **The round-trip metric** (2026-06-29 brainstorm): decompose C++ into rhizome
  markup by transclusion-to-the-limit, regenerate the source byte-identically
  (modulo formatting normalization). Every permissive open-source codebase is a
  unit test for the medium. Kept as a validation target for the text/code dataset
  kinds and sequence-kind traversal (enfilade descendants — naming.md).
- **Probe post-mortem**: the first REPL probe settled into a local minimum — line
  as (kind, address, label), byte-range addresses, edges never addressable. The
  warning stands: byte offsets only as the substrate a traversal bottoms out in;
  edges must be first-class addressable. Keep: path-to-here; a REPL as the right
  UI for design probes.

## Still open (rhizome-specific)

- Authoring hand-made connections (focus notion vs connecting among marked lines).
- Embedding vectors — LLM-derived geometry for similarity movement. Promising,
  not committed.
- Git-repo frontend; read-only FUSE adapter as a free inspection surface. UI
  adapter directions, not designed.
- The LLM-era angle, sharpened: an agent flying the link network, transcluding its
  own remixed trace through a codebase (or any corpus) to grok it — paired with
  the human doing the same. The editor and the store browser converge here.
