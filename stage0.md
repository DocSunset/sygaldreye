# stage0: the type-erased type

Working design note for the runtime C-ABI layer of stage 0 — the `syg_type` descriptor
in `src/syg.hpp`, how the reflection layer (`src/stage0.hpp`) generates one from a C++
type, and how runtime builders (`src/variant.hpp`) mint one at spawn. Captured from a
long design conversation so it survives context loss.

## What's real, and what's in flux

We committed to ONE reification: **`syg_type_t`** — **pure data** (identity + members +
layout + `template_args` + `impl`) plus exactly ONE behavior, **`tick`**. An instance is a
typed byte region held as **`syg_value_t {type, data}`** — the type rides alongside the
bytes (a fat pointer), and *a `syg_value` is its own frame*: its members are the argument
and return slots, so `tick` reads and writes them in place. Produced two ways (seam #3):
`generate_value` / `generate_component` at comptime (reflection — `src/stage0.hpp`), and
runtime builders at spawn.

**Behavior is one hot entry (`tick`) plus a cold operator registry.** `tick` is the
per-sample step, so it is cached as the type's single fn-pointer (no-op for pure data).
Everything ELSE a type can do — `place` / `erase` / `move` / `wire`, and every multi-arg
operator like `+` — is **cold** (construction, teardown, rewire) and lives NOT on the type
but in the **operator registry**, keyed by `(name, endpoints)` and resolved at build/wire
time (seam 0). So minting a type = *registering its operators*; multiple dispatch happens
once, up front, yielding a monomorphic one-`tick` type; the hot loop is always a direct
`tick`. (`syg_meta_t`/`syg_node_t` — the old node/instance structs — are long deleted.)
What remains of the old lift seam: where lift semantics (stateful /
pure / keyed) live so a scheduler can read them (seam #2).

## Members, inputs, outputs, state — all one bit

Members are **array-of-structs**, self-naming: `syg_field_t {name, type, offset}` — an
instance member, located by an offset into the object. A member's name lives once, on
the member.

A type's **template arguments** — the "statics" reframed by role, what monomorphizes it
(`constant<440>`, `latch<float>`) — are a *second* array-of-structs, `syg_template_arg_t
{name, type, addr}`. **One struct, kind read off `addr`:** non-null ⇒ a **value** arg
(`type` decodes the bytes at `addr`); null ⇒ a **type** arg (`type` IS the argument, no
storage). Value args always have storage, so null is a safe sentinel; a single array
keeps interleaved args (`<class T, int N>`) in declaration order. They fold into
`params_hash` (below): value args by their bytes, type args by their `id` — so
`meters`/`feet` (a phantom type arg over a bare `float`) share a `shape` but split on
`id`, the cheap on-ramp to refinement/sized types. Committing to this means a
*non-identity* shared datum (a cache) may **not** be a template arg — recompute it in
`place`, or feed it as a value node.

The **input/output distinction is read off the member's type, not a flag**:

- **`const T*` ⇒ input** — a borrowed edge; you read through it, you don't own it.
- **everything else ⇒ owned output/state** — a value `T`, or a `T*` you heap-own.

And **"state" is not a third category** — it's just an output the wiring reads back
(implicit `z⁻¹` feedback). The struct can't tell state from output; the graph can.

`syg_value_t {type, data}` is the typed-value primitive (a decoder + a blob); a cell,
an edge payload, an inlet default, and a template value-arg's storage are all instances.

## Authoring vs the reified ABI

Authors write **natural C++ with references**; the generator reifies each into an ABI
component where **inputs become `const T*` and everything else is owned output/state**.
(The `run()` shown below is behavior; it attaches as the type's **`tick`** method. The
member shape — inputs as `const T*`, owned outputs — is what the `syg_type` captures.)

```cpp
cell add(const cell& a, const cell& b) { return a + b; }                 // authored: free fn
struct Osc { float phase=0; cell operator()(const cell& f){ phase+=f; return isin(phase); } };

struct add_node { const cell* a; const cell* b; cell out;                // reified ABI
                  void run(){ out = add(*a, *b); } };
struct osc_node { const cell* f; Osc author; cell out;                   // author holds state (phase)
                  void run(){ out = author(*f); } };
```

Rules that make this honest:

- **References only ever appear as PARAMETERS** (bound fresh each call/tick), never as
  persistent members. A reference member can't be rebound; a `const T*` member can. So
  the ABI form uses pointers — rewirable and legally bindable — while authors keep
  references.
- **A function is just a stateless functor.** One generator rule: parameters → inputs,
  return → output(s) (a `[[=outputs]]` struct return splays to several), functor data
  members → held state. `describe_function`/`describe_component` collapse into this.
- **Inputs:** authored as `const T&` / `const T*` / by-value params, or a `const T*`
  member. **Mutable `T&`/`T&&` are forbidden** (input-purity — a borrow is never
  silently mutated). Outputs: a return, an owned value member, or an owned `T*`.
- **Mutable-reference / mutable-pointer PARAMETERS are forbidden** (out-params). Owned
  outputs need a constructor, and a function doesn't have one — so if you need custom
  output construction (heap, a resource), author a component and own it in your ctor.

## Lifecycle from ownership legibility

`place` / `erase` / `move` / `wire` are the type's RAII — but they are **registry
operators**, not fields on `syg_type` (only `tick` stays on the type). Each takes a
**`syg_value_t {type, data}`** — the object bundled with its type — so a runtime-generic
operator dispatches on the value's runtime type; a monomorphic reflected op ignores the
type and acts on `data`. Alloc/free stays generic on `size`/`align`: you `malloc`, then
resolve+apply `place`. `place` also **binds inputs** — it sets each `const T*` from a
`void** inputs` in generated, typed code (legal; no punning, no reference-rebinding).
`wire` is the per-endpoint form of that store — the ONLY legal rebind, and the live-patch
primitive. `place` writes into memory it does not own; that's accepted — the builder is a
privileged, *unsafe* kernel, and ownership safety holds for the constructed graph *while
it runs*, not during construction (every allocator/compiler is this).

Because ownership is **legible from each member's form**, the whole lifecycle is
mechanically derivable — the Rust bargain (ownership in the type ⇒ derived
drop/move/clone), via the `const`-pointer convention instead of a borrow checker:

| member | move | erase |
|---|---|---|
| `const T*` (borrowed input) | copy the pointer | nothing |
| `T` (owned value) | move the value | its dtor |
| `T*` (owned heap) | steal + null the source | free it |

`move` is what **state migration** needs (rewire = re-place + migrate; or compacting
instances). Generated *copy* is skipped — a variable-length `T*` has no length until
sized types. Caveat: `T*` = uniquely-owned is a **convention we police** (a lint), not
a compiler guarantee; aliasing an owned `T*` silently breaks the generated lifecycle.

The one law behind the gotchas: **the generator constructs only what it can**, so
everything *owned* must be default-constructible or have an authored default (outputs'
initial values, unwired inputs' default slots, the held functor).

## Identity

- **`id` = `hash_mix(scope_hash, name_hash, params_hash)`** — nominal,
  namespace-qualified, **monomorphization-aware**, stable across layout changes. Wiring
  intent, one-word compare. `name_hash`/`scope_hash` are the parts
  (same-name-across-scopes / group-by-namespace). `params_hash` = a fold over the
  **template args** — a value arg by its bytes (`addr`, sized by `type->size`), a type arg
  by its `type->id` — because the args are *what every instance of the type is made from*,
  so they belong in identity. This is the identity half of **monomorphization**: `constant<3>`
  ≠ `constant<5>`, `route 2` ≠ `route 3` — same name/scope, different args ⇒ different
  type. (Generating unique *name strings* per instantiation is the hack that avoids this;
  it only works when the value is nameable and smuggles semantics into a string. Folding
  the args is the principled form and handles a matrix or config-struct value arg as
  easily as an int.) Nothing about *instance* state enters `id` — a `constant` whose
  value were a mutable inlet default would be a single type, because then the value isn't
  a template arg.
- **`shape` = fold of the flattened, packed primitive leaves** — byte-layout only: no
  names, no scope, no offsets, platform-independent (packed wire order, not in-memory
  offsets — else peers fork at the leaves). Answers "do our bytes agree." `vec3`/`rgb`
  share a `shape`; that's correct. `constant<3>`/`constant<5>` also share a `shape` (same
  bytes ⇒ a value can flow between them) while differing in `id` — permissive-shape /
  strict-id working as intended. Concatenate `id`+`shape` for strictest sameness.
- **Permissive `shape`, strict `id`.** Same layout, different name ⇒ bytes flow, meaning
  refuses. Offsets are per-peer, in the descriptor, never in a hash.

## What actually serializes (per the book, `11-language-core`)

Only **the patch**: topology (`nodes` = id→type+params, `edges`), inlet **defaults**
(never live values), `lift_key`, subgraph inlets/outlets, plus edit **ops**. Live
values, derived structure, and **node runtime state are never serialized** — so a
node's private state may be as rich as it likes (RAII `place`/`erase` manage it). The
flat/relocatable discipline is for **shared cell payloads** (big ones are
content-addressed decoder-blobs) and **defaults**, not every component.

## FFI

`syg_type` **is** the plugin boundary: a `dlsym`'d `.so` hands back the same POD struct —
reflection-free **data** plus one `tick` pointer, its behavior otherwise resolved locally
from the registry by `id`. Reflection is just how *native* code mints one; a runtime
builder is another producer (seam #3). Layout is already C-POD; add a **version** field to
make it a real boundary (seam #4). A foreign type's *instances* run through `tick` and are
held as `syg_value_t` (type + bytes) — no intrusive header to agree on.

## impl — the type's source

`syg_type.impl` is the type's **source / definition** (declarative, not code) — ONE
meaning now that behavior moved to the registry. **Graph ⇒ the topology** (`impl.data`);
**native ⇒ its source text, or `{}`** if unspecified. It is what ships to a peer, folds
into identity, and reconstructs the type (seam #6). Orthogonal to behavior: `impl` says
*what the type is*; the registry's operators (resolved by `id`/endpoints) say *how to run
it*. So two graph-types differ only in `impl` and share the same interpreter operators; a
native type received as bare data (`impl == {}`) has its operators resolved *locally* by
`id` — types are portable, behavior is resolved-or-sourced. (The old "package of methods"
sense of `impl` is retired; the methods are registry operators.)

Native *source* (a native type carrying its own text) is **not reachable from
reflection** — reflection is semantic, not lexical: you get structure and
`source_location`, never the characters, and consteval can't read files. The split:

- **data-type source is synthesizable** — reflect the members, assemble `struct X {…};`
  at consteval (this is seam #6's reverse mapping emitting text);
- **function *bodies* are opaque** — statements aren't reflectable. Real body source needs
  preprocessor **stringization** (a `SYG_NATIVE(…)` macro that compiles the tokens AND
  captures `#__VA_ARGS__` as a string — DRY, opt-in) or external tooling slicing by
  `source_location`.

We're likely to take the macro. Its whitespace-normalization + comment-stripping — usually
a downside — is a **feature for hashing**: the normalized text content-addresses cleanly,
so a native type's source can seed its `id` the way a graph's topology seeds a graph-type's.

## The registry endgame — descriptor = term, grounding = the senses (horizon)

Not stage-0 work; the floor we found by pushing the registry idea to its limit, written
so it isn't lost.

- **A type's minimal descriptor is a *construction term*, not a record.** It is
  `(constructor, child-id, child-id, …)` — a leaf is `(prim, size, category)`; a product
  is `(product, id_x, id_y, …)` (a name-hash per child if endpoint names are in `id`).
  Everything we store in the fat `syg_type` — `size`, `align`, offsets, `shape`, `id`,
  even behavior — is **derived by folding the term**. So the struct is the *memoized fold*
  (a cache), and the term is the truth. That term IS the operator-registry key
  `(constructor-name, arg-ids)`: **descriptor = construction recipe = registry key**, one
  thing. The type universe is a **Merkle DAG of type-constructors**, content-addressed
  (`id` = hash of the term), grounding at nullary constructors (primitives).

- **Only `tick` is hot.** `size`/`align`/`members`/`template_args`/`name`/`scope`/`shape`/
  `impl` are all *build-time* (baked into offsets/topology, then never re-read). So the
  value's type-handle could shrink all the way to an **`id`**, with hot consumers (a
  graph's node array) caching resolved ticks at their use site. The irreducible native
  kernel is then just `{a hash map} + {one native descriptor layout}` — you can't go below
  "must know a descriptor's layout to read descriptors." (Mesh/portability win: ship
  `{id, data}`, resolve behavior locally. Pull it when peers ship types, not before.)

- **Grounding: self-description buys coherence, not meaning.** The fixed point (`Type :
  Type`, `string` naming string) is *bedrock*, not an explanation — it marks where the
  kernel ends, below which a reader must already know. So there is an irreducible
  **genesis vocabulary**: a small, versioned set of primitive type-ids carved into the C
  kernel and agreed *out of band* by every peer (like IPFS's multicodec table, CPython's
  `PyTypeObject`, a CPU's ISA). And the *ultimate* grounding is the **senses** — recursive
  metadata is a decoder chain whose last link is a sensation (rainfall hitting an eardrum),
  not another hash. That is Sygaldreye's thesis ("terminating in someone's senses"), and
  it means the seed is not a bug to engineer away — only a thing to carve deliberately.

Stage-0 stance: keep the fat `syg_type` struct (it is both bootstrap rock and access
cache); build a plain **exact-match** operator registry next. The term/DAG/genesis form is
the horizon.

## Open seams

0. **Behavior → operator registry (struct half LANDED, registry TBD).** The lifecycle
   fn-pointers are now REMOVED from `syg_type` (it keeps pure data + `tick` + `impl`);
   `src/variant.hpp` and `src/graph.hpp` + tests are **parked** (they need the registry to
   hold their operators). Still to build: the registry itself. Behavior becomes **operators
   in a registry**, keyed by
   `(name, endpoints)` — endpoints tagged (direction, index, type), outputs folded into
   `id` too (needs seam-0 identity). `syg_type` keeps only pure data + one cached HOT
   entry, **tick**; a `syg_value` is its own frame (members = args + return slots). Cold
   ops (`place`/`erase`/`move`/`wire`, and multi-arg operators like `+`) live only in the
   registry. Multiple dispatch is **resolve/build-time** (yields a monomorphic one-tick
   node-type); the hot loop is always a single direct `tick`. Minting a type becomes
   *registering its operators*. Stage-0 resolver = **exact match, asker supplies the name
   and the full endpoint set**; the partial-match + ranking/tie-break **query engine** is a
   known near-term want, deferred.
0b. **Endpoints in identity (seam-0, to nail down).** Fold a type's endpoints (inputs and
   outputs, by direction + index + type, maybe names) into `id`, so operator dispatch works:
   same name, different signature ⇒ different type. Load-bearing for both operators and the
   behavior-registry move above.
1. **Alignment** — `syg_type` carries `align` (widest leaf), used for inline layout (a
   variant's payload). No intrusive instance header remains (an instance is a typed byte
   region held as `syg_value_t`), so there is nothing more to pad — resolved.
2. **Lift semantics** — `run` became the type's `tick` and an instance is `syg_value_t`,
   so the node/meta structs are gone. The one leftover: where stateful / pure / keyed
   metadata lives so a scheduler can read it. A candidate is a small flags field on
   `syg_type`; undesigned.
3. **Templates** — type families from two producers: comptime
   (`generate_value`/`generate_component`, reflection over a C++ type — LIVE) and runtime
   (a builder minting the POD descriptor at spawn — prototyped in the now-parked
   `syg::variant::make`). Same abstraction, two producers — mirrors native/foreign.
4. **Versioning** — the `.so` ABI version field, and how it gates loading.
5. **Type-theory roadmap** — see `agent_notes/type-theory.md`: sums were first prototyped
   (the parked `syg::variant` — tag + cases in `template_args`); next is graph-region
   lifetimes, then sized/refinement types, added reactively.
6. **The mapping is two-way — PICK UP LATER.** We keep saying "the reflection layer
   *generates into* `syg_type`," but the relationship is a bijection we'll want both
   directions of:
   - **forward** (have): authored representation → `syg_type`. Live: a value (leaf) and a
     component struct (product); a runtime sum was prototyped (parked `syg::variant`);
     *more forms TBD* (`latch<T>`, a function's splayed outputs).
   - **reverse** (TBD): at **consteval** time, `syg_type` → an authored, type-rich
     representation — reconstruct real C++ types/members from the POD descriptor, so a
     `syg_type` known at compile time can be spliced back into typed code (typed
     `place`/`run` bodies, static asserts, generic nodes specialized on a comptime type).
   The forward direction erases; the reverse re-derives. Both matter: forward mints the
   ABI from source, reverse lets comptime code *consume* an ABI descriptor as if it were
   a native type. Open questions: what authored forms exist (the "…TBD" above), and how
   much of the reverse is expressible given a `syg_type` is runtime data — the consteval
   reverse only applies to descriptors that are themselves `constexpr`.
