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
  name editable places; an address is live if and only if it traverses a ref. No other
  mutability mechanism exists.
- **L3 (Declarations one level up).** Promises and schemas are links *about* link
  names held by the node type; instances hold the links. Checked at
  edit/commit time by a first-order oracle; never consulted at tick time.

**Data**

- **L4 (Provenance-or-fork).** Every derived thing tracks its sources or has
  recordedly detached. Never silent.
- **L5 (Two doors).** Data becomes durable only by committed derivation (recipe) or
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
- **L12 (Identity-preserving compilation).** Compilation is a deterministic derivation, committed and hence memoizable, emitting a route to route map; state survives
  re-compilation through it.
- **L13 (One definition).** Realized views are editing surfaces writing back
  through the inverse map; defaults insert only where absent; conditional
  behavior is a deliberate pass; refusal is a fork. No shadow definitions,
  ever.
- **L14 (Ports and wiring).** Engine extension points are published ports;
  packages wire in; order is wiring, visible as structure.

**Greenfield (ADR-013 through 028)**

- **L18 (The ratchet).** The core only shrinks: admission requires a
  no-composition proof (ch. 16); presence is always a tape choice; nothing
  is imposed, all the way down.
- **L19 (Errors are values; failure is death).** Noexcept by default;
  declared fault outputs; promise-breaking kills the containment unit;
  supervision policy is wiring; detection is demand-driven overlay.
- **L20 (Succession).** Nodes are succeeded, never mutated; migrations are lazy, committed derivations; compatibility is reachability; nothing is ever forced
  to rewrite.
- **L21 (The suite is the system).** Conformance defines sygaldreye; the
  suite is written in the system; blessing is testimony; the candidate is
  a peer.

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

- **Naming and resolution** (L2, L3, L7): CID/multihash; routes-from-here addresses (root:route as sugar);
  traversal per kind; normalization/memoization of fixed addresses.
- **Commit and provenance** (L4, L5): recipe and testimony headers; hashes
  only in provenance; memoization keyed by input hashes.
- **Store and fetch** (L7, L10, L15): store-as-graph access; provide/compatible
  advertisement; content-addressed, chunked, resumable fetch; back-link
  indexing at commit.
- **Compile and realize** (L11–L14): engine fan-ins; compilation maps;
  swap+migrate keyed by route and lift key.
- **Mesh session** (L15–L17): pairing, authenticated transport, three-list
  advertisement, typed refusal, plugin gate.
- **Boot** (L6, L11): trampoline to frozen stage 0 to spawn-and-park to observe,
  splice, receive, compile, realize.

## IV. Traceability

Requirements live in chapters 2–9 (`NAM, STO, EXE, CMP, SZ, PKG, MSH, EDR`).
Association, requirement to laws to needs:

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
| CMP-9 | L11, L14 | N1, N2 |
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
| EXE-10 | L7, L8 | N6, N2 |
| EXE-11 | L8, L9 | N6, N2 |
| FRZ-1 | L4, L7 | N5, N6 |
| FRZ-2 | L4 | N6, N2 |
| FRZ-3 | L7 | N6 |
| FRZ-4 | L15, L16 | N3, N8 |
| LNG-1, LNG-2 | L1, L3 | N2, N6 |
| LNG-3 | L8 | N6, N2 |
| LNG-4 | L1, L13 | N2, N5 |
| LNG-5 | L8, L13 | N1, N4, N5 |
| LNG-6 | L1, L10 | N2 |
| LNG-7 | L9 | N2, N6 |
| LNG-8 | L7 | N5, N6 |
| LNG-9 | L8 | N1 (open) |
| LNG-10 | L1, L8 | N2, N5, N7 |
| LNG-11 | L1, L3, L7 | N1, N2, N4 |
| AUT-1, AUT-2 | L7, L9 | N6 |
| AUT-3 | L3, L7 | N2, N6 |
| AUT-4 | L12 | N1, N2 |
| AUT-5 | L1, L16 | N2, N4 |
| EDR-1 | L1, L8 | N2, N1 |
| EDR-2, EDR-3 | L2, L13 | N5, N1 |
| EDR-4 | L13 | N1 |
| EDR-5 | L1, L8 | N7, N2 |
| EDR-6 | L2, L7 | N7 |
| EDR-7 | L15 | N4, N8 |
| EDR-8 | L8 | N4, N6 |
| ABI-1…5 | L3, L7, L19, L20 | N2, N6, N8 |
| FMT-1…5 | L2, L7 | N3, N5, N6 |
| TCF-1…5 | L8, L9, L19 | N6, N1 |
| COR-1…4 | L6, L11, L18 | N2, N6 |
| CNF-1…5 | L7, L20, L21 | N5, N6, N2 |
| CNF-6 | L20, L21 | N5, N2 |

*(New-chapter requirements are traced at chapter granularity; per-item rows
join as the conformance suite (CNF-1) materializes them as test datasets.)*

Reading the table backward answers "why does this requirement exist": every
row terminates in the needs, and every need traces to the vision. A proposed
requirement that maps to no law and no need should be rejected; a law that no
requirement exercises is untested doctrine and needs one.

## V. The payoff — what the finished system gives you

The needs and laws above are abstract; this is what they buy, concretely.
Every item is a consequence of the machinery in chapters 2–9, cited.

**Playing and making**

- Patch a VR world and a modular synth *while they run* — scene, sound,
  camera, and interaction rewire live, mid-performance, no restart, state
  carried across every edit (EXE-5, CMP-7).
- Every boundary the system inserts — latches, queues, network hops — is a
  visible node on the canvas you can inspect and replace, where other
  environments hide them (EXE-8). This is where we beat Max.
- Feedback, cross-rate wiring, and cross-device wiring are all just wires;
  the compiler picks the right adapter and shows its work (EXE-4, PKG-7).
- Presets are datasets: swap a sound's whole parameter face without touching
  the patch; diff two presets like code (STO-7).

**The mesh as one instrument**

- A node type living on another machine "just works" — controllers on the
  headset driving synthesis on the desktop, visuals in a browser tab
  (PKG-6).
- Capability placement: open a heavy patch on a weak device and the heavy
  regions compile onto peers that can carry them; a browser peer becomes a
  spectator of the headset's world automatically (PKG-7).
- Hot-plug a device and its whole vocabulary splices into the running
  compiler; unplug it and placement falls through to the mesh (PKG-8).
- The whole session is supervised: crash a level and its parent restarts it;
  stage 0 itself can never be edited away (SZ-5).

**Memory that behaves like memory**

- Infinite, branching undo across sessions — history is hash trails, so
  nothing is ever silently overwritten, and any version is one rebind away
  (STO-4).
- Record a take anywhere and it carries testimony: which peer, what wiring,
  signed (MSH-6). Derived artifacts carry recipes and rebuild themselves —
  re-derivation replaces backups for everything except the irreplaceable
  (STO-3, STO-5).
- Deliberate forgetting is supported: unpin everywhere and the mesh
  converges — deletion is a decision, not an accident (STO-8).
- Identical data stores once, mesh-wide, verified by hash (NAM-6).

**The compiler is yours**

- The thing that turns patches into running systems is itself a patch: open
  it, probe it, extend it by wiring into its published ports; its pass order
  is readable off the canvas (CMP-3, L14).
- Compilation is memoized — editing a parameter recompiles nothing
  structural (CMP-1).
- Freeze any patch to a standalone C++ program for the smallest targets, and
  unfreeze it later — the artifact carries its own source (L7, SZ-4).
- Offline mode: render two seconds — or two hours — of any patch as a
  provenance-carrying derivation on the worker region; the build system is
  just another region (EXE-7, PKG-5).

**Agents in the band**

- An LLM agent is a peer: it flies the camera, operates the editor, and
  reproduces your bugs through the same source nodes as your hands (EDR-7).
- The codegen loop: an agent writes a new node in C++, a worker derivation
  cross-compiles it, the artifact ships by hash with a signed provenance
  chain, and the headset hot-loads it mid-set (MSH-5, ch. 8 example).
- Agents can't hear or see the room, so the system shows them: probes,
  values, screenshots, spectrograms on any edge, without disturbing the
  patch (EDR-8).

**Sound design without asterisks**

- Feedback is one sample by default: Karplus-Strong, feedback FM, and
  filters-in-loops just work, live and interpreted; freezing only makes them
  cheap (EXE-10, FRZ-1). Explicit delays opt out — the choice is wiring.
- Polyphony is a channel count: wire a span of frequencies into one osc and
  get N keyed voices, no voice-manager node, identity preserved through
  resize (LNG-2, AUT-4).
- Freeze any patch to a portable C++ class — a plugin, a firmware image, an
  embedded instrument authored in VR — and unfreeze it back to the editable
  graph; a microcontroller running one can stay a peer, advertising its
  nodes to the mesh (FRZ-1–4).

**Horizons (validation targets, not requirements — the kind system must not
preclude them)**

- An additive synthesizer whose partials are a dataset driving an oscillator
  pipeline; atomic-decomposition analysis-resynthesis; a video
  editor-compositor; projection mapping; an ML workbench where a model
  carries provenance to its training set (ADR-006's five applications).
- The companion and spectator apps as *just more subgraphs* — expressed
  through, and hackable via, the same graph model (vision.md).
- The brainstormed application space (`kanban/applications/`, 2026-06),
  whose uniting thread is **closing the research–application gap**: a paper,
  model, dataset, or algorithm authored, explored, and shipped as an
  interactive thing in one medium — research artifact and app are the same
  patch. The notes, each riding machinery this book specifies:
  *musical instruments* (VR lutherie on the audio substrate; EXE, ADR-013),
  *game engine* and *physics sandbox* (the patch is the game / the physics
  is the content), *CAD, sculpting + slicer* and *3D scanner + LLM model
  viewer* (scan to sculpt to print; geometry as wirable content; agents
  seeing models via EDR-8-style rendered views), *robotics firmware, 3D
  printers* and *robot gardening* (one medium from motor loops to behavior —
  frozen peers, FRZ, are the firmware story), *home automation* (devices
  and sensors as edges; the mesh reaching the house), *ML authoring
  environment* ("Wekinator 2026": hand-author interpretable layers, tweak
  live, train as derivations), *code/document explorer + hypertext editor*
  and *web UI/hypertext content* (the rhizome layer's reading and authoring
  faces; EDR-6), *social appliverse* (patches as places people inhabit
  together — MSH-8's graded circles grown up).

**A medium, not just an app**

- Documents are graphs: transclude anything — a parameter, a take, a
  paragraph — as quotation (fixed) or live subscription; links are
  bidirectional and survive editing (EDR-6, NAM-7).
- Walk everything: the store browser's ground/frontier/mark turns the whole
  mesh — patches, takes, kinds, provenance — into one navigable space, and
  your walked trace is itself a committable, shareable dataset (EDR-5).
- A codebase can be decomposed into the medium and regenerated byte-identical
  — every open-source repo is a unit test for the document layer (EDR-6.2).
- The trust model reads like a patch: a peer's sandbox is literally the list
  of node names it advertises; a package is a provenance closure you can
  re-derive instead of trust (MSH-3, L15–L16).

## VI. Amendment

This chapter changes only by ratified decision, recorded in `adr.md` with the
reasoning, and reflected here in the same commit. Terminology is governed by
`planning/lexicon.md` (one name per concept, one concept per name; form vs
function; the retired-prose list is binding on all documents).
