# Chapter 10 — The Law

*The authoritative gathering of the principles, laws, and protocols the
project must achieve, the requirements it must satisfy, and the association
between the two. When a design choice and this chapter disagree, this chapter
wins or must itself be amended — recordedly.*

## I. Fundamental needs

Derived from `planning/vision.md` and the dream it serves.

- **N1 — Liveness.** Any change — feature, bugfix, extension, at any level
  including the compiler — lands without restarting. The guiding star: a
  required restart is a bug, upstream.
- **N2 — Uniformity.** Everything is expressible as nodes, graphs, and
  datasets — content, editor, compiler, infrastructure — over a finely
  curated, minimal vocabulary; composition precedes new primitives.
- **N3 — Mesh.** Multiple devices (host, headset, browser) are one
  environment: remote node types just work; placement follows capability.
- **N4 — Agency.** LLM agents are peers, not backdoors: same inputs, same
  editor, same rules; agents can extend the running system (codegen loop)
  under law.
- **N5 — Memory.** What matters is durable with honest provenance: takes,
  versions, undo, re-derivability; nothing silently lost, nothing silently
  overwritten.
- **N6 — Practicality.** Stands on existing compilers and toolchains;
  hard-real-time audio and frame-rate XR are met; no vanguard type theory in
  the runtime.
- **N7 — Medium.** The same data model carries documents and media
  (the rhizome layer) and remains honest at web scale.
- **N8 — Safety.** The mesh is a security boundary: running other people's
  graphs is safe by construction; capability escalation is deliberate and
  attributable.

## II. Laws

**Ontology**

- **L1 (One form).** Data is the only form — bytes, meaning conferred only by
  decoding. Node (container) and link (reference) are its two primal
  functions; every other concept is a role. Never reify a function as a form;
  form affords function.
- **L2 (Mutability lives in the name).** Hashes name immutable values; routes
  name editable places; an address is live iff it traverses a ref. No other
  mutability mechanism exists.
- **L3 (Declarations one level up).** Types/schemas are links *about* link
  names held by the node type; instances hold the links. Checked at
  edit/commit time by a first-order oracle; never consulted at tick time.

**Data**

- **L4 (Provenance-or-fork).** Every derived thing tracks its sources or has
  recordedly detached. Never silent.
- **L5 (Two doors).** Data becomes durable only by derivation (recipe) or
  capture (testimony). Streams cannot promise immutability; commitment is an
  act.
- **L6 (Ground).** Every regress ends in declared ground: sacred kinds,
  captures, natives, stage 0, the header convention — fiat and testimony,
  recorded as such. Below stage 0, provenance continues as build derivations.
- **L7 (Format is law).** The at-rest form (bags of links, datasets) is the
  contract; every runtime representation is an observationally equivalent
  cache that must round-trip.

**Execution**

- **L8 (Derive, don't mutate).** Ticking is uncommitted derivation;
  instantiation is uncommitted derivation in space; commit turns either into
  a dataset. Regions are inferred, never declared; boundaries are reified as
  visible, replaceable mappings.
- **L9 (Executors own pacing).** Every region's cadence and platform resource
  belong to its executor node; no node reaches around its region's contract.
  Real-time regions allocate nothing after init; resources acquire on first
  tick, never in create().
- **L10 (Existence is reference).** Instances exist while linked; datasets
  exist while provided. Creation and destruction are link operations plus
  collection; there is no other lifecycle.

**Compilation**

- **L11 (Roles, not types).** App/engine/execution are roles in a
  compilation relationship; every graph is defined, compiled, realized —
  including engine graphs. The tower is lazy and grounds at the frozen
  stage 0, whose sole irreducible property is pre-existence.
- **L12 (Identity-preserving compilation).** Compilation is a deterministic,
  memoizable derivation emitting a route→route map; state survives
  re-compilation through it.
- **L13 (One definition).** Realized views are editing surfaces writing back
  through the inverse map; defaults insert only where absent; conditional
  behavior is a deliberate pass; refusal is a fork. No shadow definitions,
  ever.
- **L14 (Ports and wiring).** Engine extension points are published ports;
  packages wire in; order is wiring, visible as structure.

**Mesh**

- **L15 (Advertisement is permission).** Placement is pull-shaped; a peer
  runs, serves, and subscribes to exactly what it publishes. Selective
  advertisement is the sandbox; only vocabulary and refs are ever gated,
  never the data model.
- **L16 (Graphs flow, plugins are gated).** In-mesh graphs realize freely
  within advertisement; native code is a distinct, provenance-gated,
  signed act.
- **L17 (Keys, not secrets).** Identity is per-peer keypairs; testimony and
  plugins are signature-verifiable; membership is revocable; secrecy, when
  built, is decoder scarcity.

## III. Protocols

The system, in one sentence (vision.md): *a protocol for encoding recursive
metadata and decoding data to derive other data — grounded in stage 0, paced
by executors, shared by a mesh, terminating in someone's senses.* Its
concrete protocol surfaces, each governed by the laws cited:

- **Naming & resolution** (L2, L3, L7): CID/multihash; root+route addresses;
  traversal per kind; normalization/memoization of fixed addresses.
- **Commit & provenance** (L4, L5): recipe and testimony headers; hashes
  only in provenance; memoization keyed by input hashes.
- **Store & fetch** (L7, L10, L15): store-as-graph access; provide/compatible
  advertisement; content-addressed, chunked, resumable fetch; back-link
  indexing at commit.
- **Compile & realize** (L11–L14): engine fan-ins; compilation maps;
  swap+migrate keyed by route and lift key.
- **Mesh session** (L15–L17): pairing, authenticated transport, three-list
  advertisement, typed refusal, plugin gate.
- **Boot** (L6, L11): trampoline → frozen stage 0 → spawn-and-park → observe,
  splice, receive, compile, realize.

## IV. Traceability

Requirements live in chapters 2–9 (`NAM, STO, EXE, CMP, SZ, PKG, MSH, EDR`).
Association, requirement → laws → needs:

| Requirement | Laws | Needs |
|---|---|---|
| NAM-1, NAM-2 | L2 | N2, N5, N3 |
| NAM-3 | L2, L12 | N1, N5 |
| NAM-4 | L1, L3 | N2, N7 |
| NAM-5 | L3, L8 | N2, N6 |
| NAM-6 | L7 | N5, N7, N3 |
| NAM-7 | L2, L7 | N7, N5 |
| STO-1, STO-6 | L7 | N5, N3 |
| STO-2 | L1, L15 | N2, N8 |
| STO-3 | L4, L5 | N5 |
| STO-4 | L2, L4 | N5, N1 |
| STO-5 | L10, L15 | N5, N3 |
| STO-7 | L1, L13 | N2, N5 |
| STO-8 | L4, L10 | N5, N8 |
| STO-9 | L4 | N5, N7 |
| EXE-1 | L7 | N6, N2 |
| EXE-2 | L8 | N2, N6 |
| EXE-3 | L9 | N6 |
| EXE-4 | L8 | N6, N2 |
| EXE-5 | L12 | N1 |
| EXE-6 | L9 | N2, N6 |
| EXE-7 | L5, L8 | N5, N2 |
| EXE-8 | L8, L13 | N1, N2 |
| EXE-9 | L10 | N2 |
| CMP-1 | L4, L12 | N5, N6 |
| CMP-2 | L12 | N1 |
| CMP-3 | L14 | N2 |
| CMP-4 | L13 | N1, N2 |
| CMP-5 | L4, L13 | N5 |
| CMP-6 | L11 | N6 |
| CMP-7 | L12 | N1 |
| CMP-8 | L7, L11 | N6 |
| SZ-1, SZ-6 | L11 | N2, N6 |
| SZ-2 | L6, L15 | N2, N8 |
| SZ-3 | L6 | N6, N5 |
| SZ-4 | L4, L6 | N5, N6 |
| SZ-5 | L11 | N1, N6 |
| SZ-7 | L6 | N6, N3 |
| PKG-1 | L14 | N2 |
| PKG-2 | L7, L9 | N6 |
| PKG-3, PKG-4 | L8, L9 | N1, N2 |
| PKG-5 | L5, L8 | N3, N5 |
| PKG-6 | L8, L15 | N3 |
| PKG-7 | L12, L14 | N3, N2 |
| PKG-8 | L14 | N1, N3 |
| MSH-1, MSH-2 | L17 | N8, N3 |
| MSH-3, MSH-4 | L15 | N8, N3 |
| MSH-5 | L16 | N8, N4 |
| MSH-6 | L17, L5 | N8, N5 |
| MSH-7 | L15 | N3, N7 |
| MSH-8 | L15, L17 | N8, N7 |
| EDR-1 | L1, L8 | N2, N1 |
| EDR-2, EDR-3 | L2, L13 | N5, N1 |
| EDR-4 | L13 | N1 |
| EDR-5 | L1, L8 | N7, N2 |
| EDR-6 | L2, L7 | N7 |
| EDR-7 | L15 | N4, N8 |
| EDR-8 | L8 | N4, N6 |

Reading the table backward answers "why does this requirement exist": every
row terminates in the needs, and every need traces to the vision. A proposed
requirement that maps to no law and no need should be rejected; a law that no
requirement exercises is untested doctrine and needs one.

## V. Amendment

This chapter changes only by ratified decision, recorded in `adr.md` with the
reasoning, and reflected here in the same commit. Terminology is governed by
`planning/lexicon.md` (one name per concept, one concept per name; form vs
function; the retired-prose list is binding on all documents).
