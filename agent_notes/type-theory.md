# Type theory of stage 0 — what we built, and what's stronger

A type-theoretic reading of the `stage0.md` design (the `syg_type_t`/`syg_meta_t`/
`syg_node_t` layer). Answers: what did we implement, and what would it cost/gain to
go stronger. Companion to `../stage0.md`.

## What we actually have

Roughly **the type system of C structs**: **simply-typed, first-order, monomorphic,
product-only, nominal**, over a set of primitive base types, closed by
recursion-into-fields (finite — it grounds out at primitives).

- **Product-only, nominal.** Types are records (named fields) over base types;
  identity is nominal (`id` = hash of scope+name), with a separate structural
  fingerprint (`shape`) available as a byte-compat check. No sum types (variants),
  no first-class function types (functions are `run`, erased to `void(*)`), no
  polymorphism, no kind system.
- **Recursion is limited.** Recursion *through a reference field* is expressible
  (iso-recursive via a borrow). Genuinely-recursive *data* (a cons-list) is not —
  that needs sums + indirection.

Two layers sit on top, both **weaker than they look**:

- **Ownership is Rust-shaped but unchecked.** "One owning cell per value; references
  are shared immutable borrows (`&T` — many readers, no mutation)" is **affine
  ownership with shared borrows** (drop-without-use is allowed ⇒ affine, not linear).
  But there is **no checker**. It's the *shape* of a substructural system as an
  architectural discipline, not a proof. The open seam "a borrow must not outlive its
  producer" is exactly the theorem we are **not** proving — we punted it to "the
  scheduler's job."
- **Dependency exists only via monomorphization.** `route N` (arity from a creation
  arg) is a value-indexed type — a poor man's Π — but resolved at *spawn* into a
  concrete monomorphic type. That's **staging**, not dependent typing. The span/rank
  machinery (`lng.span_semantics`) is dynamic runtime shape-indexing, unchecked.

And crucially: we implemented the type *objects* plus **one judgment** — nominal
equality at `connect` (`id` match). No inference, no subtyping, no soundness. It is a
representation with a single equality check, **not a type theory with rules**.

## Going stronger — the axes

Each strengthening migrates a class of runtime discipline into a static checker. The
field's lesson (Rust = affine+regions, not linear logic; Liquid Haskell =
refinements, not Agda) is that most of the safety lives in a *restricted* form at a
fraction of the cost.

| Add | Cost | Gain | Cheap restricted form |
|---|---|---|---|
| **Sums / variants** (+ recursion) | Low — tag + dispatch in `shape`/`place`/`run` | Events with payloads, optionals/results, real ADTs, recursive data (lists/trees) | just tagged unions; nominal `id` already handles distinctness — **do first** |
| **Static substructural** (affine + regions/lifetimes) | High | The dangling-edge / use-after-free-of-dead-producer *proof*; safe hot-patching — our #1 live-patch risk, machine-checked | **graph-level region checking** — the DAG makes producer/consumer edges explicit, so "does this borrow outlive its source" is a *structural* check, far cheaper than full Rust |
| **Real dependency** (Π/Σ over values) | Very high | Buffer-length / sample-rate / channel-count mismatches become *type errors* — enormous for DSP | **refinement / sized types** (Liquid-style) — indices are naturals + linear arithmetic, decidable; the span/rank stuff already begs for it |
| **Polymorphism + kinds** (templates done right) | Moderate | One `latch`/`route` for all types; kills hand-written near-duplicate nodes | **monomorphized generics** — C++ templates already give this; skip first-class ∀ and a formal kind layer |

## Recommendation

Chase the **restricted forms, reactively.** Payoff order for this project (live
audio/visual/XR dataflow + mesh + hot-patching):

1. **Sums** — cheap, unlocks events/optionals/recursive data.
2. **Graph-region lifetimes** — turns the #1 open correctness risk into a proof, and
   the explicit DAG makes it tractable (much cheaper than general borrow-checking).
3. **Sized / refinement types** — dimension/rate/channel safety, where dependency
   actually earns its keep in DSP.
4. **Monomorphized generics** — opportunistically, via templates.

Full dependent or full linear types would be a research project that constrains what
authors can express, for guarantees we can get ~90% of otherwise. The one place *not*
to skimp is **regions/lifetimes** — it's the difference between "the scheduler is
careful" and "dangling edges are unrepresentable," which is the property a
live-patchable mesh most wants.

## Bottom line

We built the floor — simple nominal products + one equality judgment. Every storey up
is runtime discipline migrated into a checker. Add each storey the moment a bug-class
or a reach-for-C++ shows the graph can't say something — not before.
