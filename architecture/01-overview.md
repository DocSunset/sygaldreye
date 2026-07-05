# Chapter 1 — Architectural overview

*For those who will implement and maintain sygaldreye. Read the glossary
(ch. 0) first; every italicized term is defined there. Chapters 2–9 break the
architecture into parts with enumerated requirements; chapter 10 is the law.*

## 1. What this system is

> **Sygaldreye is a protocol for encoding recursive metadata and decoding data
> to derive other data — grounded in stage 0, paced by executors, shared by a
> mesh, terminating in someone's senses.**

The near goal is a live-patchable audio/visual/XR environment spanning a Linux
host, a Quest headset, and browser peers, where *everything* — scene, sound,
camera, the editor itself, the compiler itself — is editable at runtime
through one graph model. The far goal is a general medium: the same data model
carrying documents, media, analyses, and ML artifacts (the rhizome layer,
ch. 9), honest at web scale (ch. 8).

Two guiding stars, equal in rank, binding on every design decision and
every line of implementation:

1. **Any time we must restart the app to change or extend it, we are
   failing a crucial test.** Always ask what would need to have been
   changed before, so the change could land without restarting — and fix
   that.
2. **Make graphs, not C++.** Every new behavior defaults to a graph; a
   native requires a declared clause (ADR-033), and anything called a
   graph is realized through the one contract (ADR-034). When something
   cannot yet be authored as a graph, the missing vocabulary is the bug:
   build the reusable pieces as nodes so that it can — then author it
   (L22).

## 2. The running example

`hello-cosine`: a 220 Hz cosine, amplitude waved by a 0.5 Hz LFO, into the
speakers. At rest it is four nodes in a store:

```
ref  graphs/hello-cosine ──→ #a11

#a11  kind → graph                       (composite node)
      topology → #b22 · defaults → #c33
      provenance → (capture: authored by travis@#key)

#b22  kind → graph-topology
      nodes/osc0  type → osc      nodes/lfo0  type → lfo
      nodes/vca0  type → vca      nodes/dac0  type → dac
      edges/0  osc0/out → vca0/in
      edges/1  lfo0/out → vca0/gain
      edges/2  vca0/out → dac0/in

#c33  kind → graph-defaults
      osc0/freq → 220 · osc0/shape → "cosine" · lfo0/freq → 0.5
```

Its journey to sound — resolve, decode, compile, realize, tick, transduce — is
traced through every chapter; the compiled result places `osc0–vca0–dac0` in a
block region under an audio executor, inserts a visible `latch` where the
frame-rate LFO meets the block-rate gain, and survives any edit without a
restart.

## 3. The ontology (one form, functions all the way up)

Everything is **data** — bytes, meaning something only to a compatible reader
through a decoder. Data has exactly two primal functions: read as a container
it is a **node** (a bag of named links); read as a reference it is a **link**.
Every other concept in this book — dataset, kind, type, instance, state, ref,
mapping, executor, store, document — is a *role* some node or link plays,
conferred by usage, context, or history. Two rules police this: never reify a
function as a form ("there are no LFOs, only oscillators patched slow"), and
form affords function (the kind system catalogs affordances; it is not a
taxonomy of roles).

Names come in two kinds, and **mutability lives in the name**: a *hash*
(content-derived) names an immutable value; a *route* (composed of
container-conferred local names) names a place whose content may change. An
*address* = root + route; it is *fixed* if and only if it traverses no ref, else *live* —
a subscription. This one rule generates the store's immutability, the
patch's editability, undo (a ref's hash trail), quotation versus subscription,
and the identity story for compilation — with no special cases.

## 4. The data model (chapters 2–3)

A node's data becomes durable by **commit**, through exactly two doors:
**committed derivation** (a recipe: inputs are hashes; re-derivable, memoizable, evictable) and **capture** (input was the world; provenance is
testimony; irreplaceable). The law relating derived data to sources is
**provenance-or-fork**: track, or detach recordedly — never silently. Every
derivation chain bottoms out in captures and fiat-named nodes: the ground.

The **store is a graph** of dataset nodes — plural, nested, scoped, reached
only by wiring (lexical, never ambient). Reading is transclusion; writing is a
graph edit; a ref is a node; queries are traversal. Availability reuses the
capability model: peers **provide** datasets (a kept, advertised copy) or are
merely **compatible**; durability = someone provides it; there is no
distributed GC — caches evict freely because derived data re-derives and
captures pin on provide-everything peers. Formats are IPFS's (CID/multihash,
Merkle-DAG chunking); transport is mesh-local; provenance is Nix-shaped.

## 5. Execution (chapter 4)

At runtime, the bag of links is never executed — it is **compiled into a
cache**: a plan with links resolved to raw pointers, defaults applied,
state structs allocated. Ticking a plan is an *uncommitted derivation*;
streaming is derivation without commitment. Port promises (kind and discipline) are checked at edit time by a plain first-order oracle and consulted never at tick time. This is
the practicality doctrine: **the dumb model is the format and the law; every
runtime is an observationally-equivalent compiled cache of it** (git's
packfiles; Nix's bash). C++ (PFR endpoint structs, kernels, shells) is the
authoring surface; declarations are generated from it.

Rates (event/frame/block) induce **regions** — inferred, never declared — and
where regions meet, compilation inserts visible, replaceable **mappings**
(latch, snapshot, queue, ring, net, z-inverse). An **executor** node owns each
region's pacing and platform resource (audio device callback, XR frame loop,
worker thread); inside it the whole story recurses.

## 6. Compilation and the tower (chapter 5)

App, engine, and execution graph are **roles**: every graph is defined,
compiled by an engine graph, realized on hardware — including engine graphs
themselves. The tower is lazy (levels instantiate only when edited) and
grounds at stage 0. Compilation is deterministic and emits a **compilation
map** (app route to execution route) so state survives re-compilation — the
main engineering risk, and the enabler of **projection editing**: realized
views are editing surfaces whose edits write back through the inverse map;
conditional-on-compilation behavior is a deliberate pass; refusing write-back
is a fork. The engine pipeline's extension points are **ports** (fan-ins:
recognize-region, construct-context, choose-adapters) and **order is wiring**.

## 7. Stage 0 and boot (chapters 6, 16)

The core, ratcheted to its floor (ADR-028): **the escapement** (node
contract + tick — the only unconditional substrate; a sealed firmware is
escapement + a frozen **movement**), **the crown** (mutable plan + one op
applier + an inlet — the minimal self-modification, fed at boot by a **tape**
of op records, no parser needed), and **complications** — everything else,
including the bootloader's own organs, present only by tape choice. Stage 0
is a frozen realization of that boot tape (a Nix derivation; provenance into
the flake lock); its irreducible property is pre-existence. It spawns the
engine graph and parks as the one supervisor that cannot be edited away.
Platform entry is a <10-line trampoline (including `import sygaldreye`);
stage-0 logic never branches on platform.

## 8. Capabilities, mesh, trust (chapters 7–8)

Platform features arrive as **capability packages**: vocabulary (node types,
present by linkage) + passes (wired into engine fan-ins) + machinery (the C++
inside executor natives). The audio region is the prototype; XR, render,
worker, store, and net follow the same shape. When a capability is absent
locally, compilation places that region **on a peer advertising it** —
cross-network placement is a fallthrough of compilation, not a feature.

The mesh is a security boundary. Identity is **per-peer keypairs** with a
pairing ceremony (revocation; unforgeable capture testimony; signable
provenance). **Advertisement is the permission system** — placement is
pull-shaped; selective advertisement is the sandbox. Graphs flow freely
in-mesh; **native plugins are a distinct act** gated by provenance policy. At
scale, membership grades into circles (devices / collaborators / world); only
vocabulary and refs are ever gated — never the data model.

## 9. The surfaces (chapter 9)

The editor is graphs editing graphs — cards, wires, gestures, palette are
nodes (already true in the codebase). The store browser inherits the astui
navigation model (here / path / frontier / mark). Documents are graphs;
transclusion is an address-valued edge; every document's "proposed program to
render" is a declinable graph run in the reader's advertised vocabulary.
Agents are peers, not backdoors: they act through the same source nodes as a
hand, and the LLM codegen loop (write node to cross-compile to ship to live-load)
rides the plugin channel under provenance policy.

## 10. How to read the rest

| Chapter | Part | Prefix |
|---|---|---|
| 2 | Data, naming, kinds, types | NAM |
| 3 | Store, commit, availability | STO |
| 4 | Execution: plans, regions, mappings, executors | EXE |
| 5 | Compilation, tower, projection editing, freezing | CMP / FRZ |
| 6 | Stage 0 and boot | SZ |
| 7 | Capability packages | PKG |
| 8 | Mesh and trust | MSH |
| 9 | Editor and documents | EDR |
| 10 | Laws ↔ requirements traceability | LAW / N |
| 11 | The language core | LNG |
| 12 | Node authoring & conformability | AUT |
| 13 | The native contract | ABI |
| 14 | Formats & wire protocols | FMT |
| 15 | Time, concurrency, memory, faults | TCF |
| 16 | The Core (escapement · crown · complications) | COR |
| 17 | Conformance & evolution | CNF |
| A | The greenfield build order | — |

Each part chapter gives context, design, **enumerated requirements**
(`XXX-n`), **acceptance criteria** (`XXX-n.m`, phrased to become automated
tests), and worked examples. Chapter 10 states the fundamental needs
(`N1–N8`) and laws and maps every requirement to them. ADR-013 through 028 (the
greenfield session, 2026-07-03/04) govern throughout.
