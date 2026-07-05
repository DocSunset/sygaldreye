# Chapter 16 — The Core (COR)

*The ratchet's ledger: what the core is, exactly, and the process that keeps
it shrinking. Governing ADRs: 024, 026, 028. This chapter IS the core
membership document ADR-026 requires.*

## The two-axis rule

Two orthogonal properties, never conflated again:

- **Core-named** — passes the no-composition test: nothing expressible as a
  composition of other vocabulary may bear the name. Governed by this
  chapter.
- **Present** — linked into a given target and instated by its tape/boot
  graph. ALWAYS a per-target choice; nothing is imposed (ADR-016/028).

## Stratum 0 — the escapement (unconditional)

The node contract (ch. 13) + tick-in-order. A calling convention and a
for-loop; constexpr-able. The only thing every sygaldreye artifact contains.
A **movement** is a frozen realized graph the escapement ticks.

## Stratum 1 — the crown (the minimal self-modification)

The plan as mutable data · one applier primitive (instantiate-linked-native,
link, unlink, remove, write-default; applied at tick boundaries) · an op
inlet. Boot input is the **tape** (FMT-3). The full slot is the crown grown
up. A sealed movement omits the crown entirely.

## Stratum 2 — the specs (definitional, code nowhere)

naming · canonical encoding · kind/type model · three disciplines ·
provenance grammar (derive/capture, determinism classes, succession) ·
edit-op vocabulary · the native contract · the wire protocol · the
compilation contract · the conformance harness shape. Each is a spec node,
succeeded like anything (ADR-025); the CID header convention alone is
frozen by design.

## Stratum 3 — the core-named vocabulary (nodes like all nodes)

Uncomposable, hence core-named; present only by choice; these are the
liveness organs a live peer's tape instates:

| name | why uncomposable |
|---|---|
| slot | restructures the runtime cache transactionally with state migration |
| ref | the one rebindable link |
| subgraph | clone-behind-trampolines composition |
| reflection seam | a node seeing its enclosing graph (context injection) |
| traverse · filter · join · fixpoint | the query four (ADR-024) |
| z⁻¹ · latch · snapshot · queue · ring · net · probe | the seven mapping *definitions* — their guarantees are core semantics (TCF-1); implementations ride executors |
| parser · codec · naive resolver · registry-face · loader | the decode organs: each crosses data↔machinery, which composition cannot |

Nineteen names. Everything else in the universe — every executor, every
kind in the catalog (data!), the engine pipeline (data!), math, the store,
the mesh, the freezer, the editor, fault handling — is a **complication**.

## The ratchet process (ADR-026 §3)

- Admission: a written no-composition proof, recorded here and in adr.md,
  in the same commit.
- Standing **shrink list** (candidates for eviction when composition
  catches up): reflection seam (possibly a standing query over the live op
  stream); parser (a graph ≡ its ops — the tape already bypasses it; a
  parser may reduce to a projection decoder); registry-face (possibly a
  store query over a frozen store-graph).
- Review trigger: any new admission requires re-testing the shrink list.

## Engine v0 (data, not core — recorded here for the greenfield build)

The initial engine pipeline ships as a dataset: `receive → compile →
realize`, publishing the three fan-ins (recognize-region,
construct-context, choose-adapters) and the realize backends (interpret ·
codegen). CMP governs its contract; packages populate it.

## Requirements

**COR-1 (escapement austerity).** The escapement compiles freestanding;
symbol audit shows no vocabulary, no codec, no allocator beyond the
target's chosen pool.
**COR-2 (crown sufficiency).** From escapement + crown + linked liveness
organs + a tape, a peer reaches full liveness (parser, store, mesh
instated) with zero code paths outside op application — the bootstrap
ladder test (extends SZ-7).
**COR-3 (ratchet enforcement).** CI gate: the core-named list in this
chapter matches a machine-readable manifest; a PR adding a name without a
proof section fails.
**COR-4 (two profiles).** Movement-level and peer-level conformance run
against every target (ch. 17); a sealed movement passes movement-level with
the crown absent (TCF-5).
