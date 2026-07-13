# stage0: the type-erased node

Working design note for the runtime C-ABI layer of stage 0 — the structs in
`src/syg.hpp`, the shape the reflection layer (`src/stage0.hpp`) will generate into,
and the shape a foreign `.so` hands back. Sketch-stage; the code in `syg.hpp` is the
target, not yet wired to reflection. Captured from a long design conversation so it
survives context loss.

## The three reifications

One thing described three ways, each a distinct POD struct (`syg.hpp`):

- **`syg_type_t`** — a **type**: pure data description (identity + members + layout)
  plus RAII (`place`/`erase`/`move`). One static instance per type; shared.
- **`syg_meta_t`** — a **node**: a type plus the one thing data can't do, `run`. The
  shared class/vtable, one per component type.
- **`syg_node_t`** — an **instance**: an intrusive `const syg_meta_t*` header followed
  by the component's state. A running node is one pointer, self-describing (recover
  its class with `meta_of` — a first-member cast).

A node **is** a type — its members are its endpoints — so almost everything
describable lives in the type; the node adds only `run`.

## Members, inputs, outputs, state — all one bit

Members are **array-of-structs**, self-naming: `syg_field_t {name, type, offset}` for
instance members, `syg_static_t {name, type, addr}` for shared class-level ones (an
instance member is located by an offset into the object; a static by an absolute
address — the *only* difference). A member's name lives once, on the member.

The **input/output distinction is read off the member's type, not a flag**:

- **`const T*` ⇒ input** — a borrowed edge; you read through it, you don't own it.
- **everything else ⇒ owned output/state** — a value `T`, or a `T*` you heap-own.

And **"state" is not a third category** — it's just an output the wiring reads back
(implicit `z⁻¹` feedback). The struct can't tell state from output; the graph can.

`syg_value_t {type, data}` is the typed-value primitive (a decoder + a blob); a cell,
an edge payload, an inlet default, and a static's storage are all instances of it.

## Authoring vs the reified ABI

Authors write **natural C++ with references**; the generator reifies each into an ABI
component where **inputs become `const T*` and everything else is owned output/state**:

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

`place`/`erase` are the only irreducible type behavior (ctor / dtor). Alloc/free stays
generic on `size`: `create = malloc(size) + place`, `destroy = erase + free`. `place`
also **binds inputs** — `place(mem, void** inputs)` sets each `const T*` from
`inputs[i]` in generated, typed code (legal; no punning, no reference-rebinding). No
`connect` primitive is needed; dynamic rewire, if ever wanted, is the same generated
`n->in_i = (const T*)src` write.

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
  (same-name-across-scopes / group-by-namespace). `params_hash` = a fold over each
  **static member's bytes** (`data`, sized by `type->size`) — because a static is, by
  definition, frozen-and-shared, i.e. *part of what every instance of the type is*, so
  it belongs in identity. This is the identity half of **monomorphization**: `constant<3>`
  ≠ `constant<5>`, `route 2` ≠ `route 3` — same name/scope, different frozen statics ⇒
  different type. (Generating unique *name strings* per instantiation is the hack that
  avoids this; it only works when the value is nameable and smuggles semantics into a
  string. Folding the bytes is the principled form and handles a matrix or config-struct
  static as easily as an int.) Nothing about *instance* state enters `id` — a `constant`
  whose value were a mutable inlet default would be a single type, because then the value
  isn't static.
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

`node` **is** the plugin boundary: a `dlsym`'d `.so` hands back the same POD structs
because they're reflection-free data + function pointers. Reflection is just how
*native* code mints one. Layout is already C-POD; add a **version** field to make it a
real boundary. A stateful foreign node places itself into a byte region we own — so a
separate opaque `binding` survives only for a foreign node that insists on owning its
own heap and hiding its size.

## Open seams

1. **Node-instance header alignment** — if a component needs alignment > pointer, the
   header pads to `alignof(max)`; `state(n)` accounts for it.
2. **Node-level metadata** — lift semantics (stateful / pure / keyed) ride on
   `syg_meta`; undesigned.
3. **Templates** — type families minted comptime (reflection over a C++ template) or at
   runtime (build the POD descriptor at spawn, for generic-lifecycle nodes like
   `route`). Same abstraction, two producers — mirrors native/foreign.
4. **Versioning** — the `.so` ABI version field, and how it gates loading.
5. **Type-theory roadmap** — see `agent_notes/type-theory.md`: sums, then graph-region
   lifetimes, then sized/refinement types, added reactively.
6. **The mapping is two-way — PICK UP LATER.** We keep saying "the reflection layer
   *generates into* `syg_type`," but the relationship is a bijection we'll want both
   directions of:
   - **forward** (have): authored representation → `syg_type`. Input is a function, a
     component struct, or *more forms TBD* (a variant's cases table, a value node's
     static, …).
   - **reverse** (TBD): at **consteval** time, `syg_type` → an authored, type-rich
     representation — reconstruct real C++ types/members from the POD descriptor, so a
     `syg_type` known at compile time can be spliced back into typed code (typed
     `place`/`run` bodies, static asserts, generic nodes specialized on a comptime type).
   The forward direction erases; the reverse re-derives. Both matter: forward mints the
   ABI from source, reverse lets comptime code *consume* an ABI descriptor as if it were
   a native type. Open questions: what authored forms exist (the "…TBD" above), and how
   much of the reverse is expressible given a `syg_type` is runtime data — the consteval
   reverse only applies to descriptors that are themselves `constexpr`.
