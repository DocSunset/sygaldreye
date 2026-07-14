# Status

_Vision: `vision.md`. How we work: `AGENTS.md` → "How we build now". The
book: `architecture/`. Machine gates (they TRAIL): `python3 conformance/run.py`._

## RESUME BLOCK (keep current at every stopping point)

- **Where we are:** the **third draft**, just begun (2026-07-07). `src/` is
  an empty frame (`src/syg/main.cpp` dispatches nothing yet). The book,
  conformance suite, and `adr.md` are carried verbatim at root from probe2 —
  the SPEC is unchanged; only the implementation was thrown away. Both prior
  implementations are deprecated probes: `probe/probe1` (the working C++/VR
  monolith, pre-ontology) and `probe/probe2` (the greenfield that reached
  163-green + rung-12 self-hosting in a day, unwatched — reference and
  salvage; much of the third draft may be it ported forward WITH explanation).

- **Why we restarted (the pivot, 2026-07-07):** probe2 went green without
  anyone able to say in their heart it was the thing we described — still
  10k+ lines of C++, some natives that had no right being C++, built with
  ~zero human oversight. This draft is paced by **Travis reading and signing
  every line.** The real project is changing Travis's brain, not shipping;
  patience is the discipline, speed is not an acceptance criterion. Full
  norms in `AGENTS.md` — read them before writing code.

- **Built so far (2026-07-09), all in `src/escapement/`, all verified on
  GCC 16.1 (greenfield flake — NOT probe1's; reflection needs `-freflection`
  + `<meta>`):**
  - Runtime core (plain templates, no reflection): `cell` (intptr), `word` =
    `void(*)(void** in, void** out)` (multi-output), `node` descriptor
    (in_count/in_sizes/out_count/out_sizes/fn), `binding` = `{word, void**
    inputs, void** outputs, operator()}`, `escapement`/`tick` (tick an array
    of binding* — `(*movement[i])()`). `describe<fn>()` erases a free function
    to a node via template deduction (NOT P2996). Input-purity rule: value or
    `const T&` only; no mutable/rvalue ref.
  - `plan.hpp`: `bump` (arena alloc) + `make_binding` — a **self-owning
    binding** (inputs nullptr, outputs own their cells, sized at runtime from
    the descriptor). Arena comes from the `mem_malloc` word.
  - `crown.hpp`: replays a flat op tape (`N`/`L`/`S` records) into an arena of
    self-owning bindings; `LINK`/`SET` are pointer-wiring. No flat state
    buffer. (probe1's model, type-erased.) Kinds/ids are indices for now
    (registry/resolver = later); constants cell-sized.
  - `component.hpp` (reflection layer, needs `-freflection`): `component<^^fn>`
    generates a type-RICH node (named typed members, owned output, operator())
    — the thesis's component; and `describe_component<C>()` erases an authored
    component (const-ref inputs, value outputs) back to a node.
  - **Key synthesis:** `binding` and `component` are one form at two type
    levels (erased vs typed); `call` is the binding's operator(); the state
    array exists only because erasure can't size outputs inline. See ADR-038
    and the 2026-07-09 documentary entry.

- **Stage-0 TYPE-SYSTEM thread — the live design front, separate from
  `src/escapement/`. NEW PROTOCOL (2026-07-13): Travis is pair-designing —
  sketch C++ IN MESSAGES ONLY, land to disk + commit ONLY on his explicit go.**
  - **THE KERNEL CONSOLIDATION (2026-07-13/14 session — supersedes parts of
    `stage0.md`, which still documents the fat-`syg_type` stance):** everything
    in the universe is a **node** in ONE table. `syg_value`/`syg_type` merge:
    a type is a node whose `type` is its constructor and whose `data` is its
    construction TERM; constructors (prim/ptr/struct/variant) are nodes too
    (open set, registered); the registry is itself a stateful node
    (content-addressed insert = confluent). Behavior left the header: the tick
    of type T binds via a **ref** (ch. 2 NAM — per-peer, rebindable,
    subscription ⇒ hot-swap falls out); content-addressed vs ref = fixed vs
    live, already ratified. **Landed + committed (`6c46b7e`, `925a2ba`):**
    `src/hash.hpp` = `hash64_fnv1a` (digest member + str/mix/bytes statics +
    basis/prime, ONE struct = decree-v1 identity; `using syg_hash =` is the
    migration seam; decree-duty idea notes in header comment) and
    `src/node.hpp` = `syg_node_t {id, type, size, data}` (THE decree struct;
    portable half {id,type,size}+bytes) + `syg_id` = fnv1a(type ∥ size ∥
    bytes), u64s LSB-first, PINNED. `syg.hpp`/`stage0.hpp` migrated onto
    `syg_hash::` (digest values unchanged; `leaf_category` returns u64 code).
    **The eternal decree surface (to write down as a versioned doc):** preimage
    spelling + algorithm (+ digest type IS a genesis prim) + genesis roster
    (prims fixed-width only; cstr KILLED — size lives in the header; category
    KILLED — leaf identity does its job; void* = ptr(nil)) + the doc versions
    itself. Next decree draft = mine to write, his to ratify.
  - **KNOWN BREAKAGE (pre-existing, escapement thread):** `plan.test.cpp`
    calls `binding::slots()` which doesn't exist — fails the full build on
    HEAD. Untouched by this session; fix on the escapement thread.
  - Earlier layer (2026-07-12/13): `syg_type`, the POD type descriptor.
  - **On disk:** `src/syg.hpp` = `syg_type_t` (PURE DATA now: id/name/scope/shape
    hashes, name, scope, size, align, members[], template_args[], ONE `tick`
    fn-pointer, `impl`) + `syg_value_t {type,data}` + `syg_field_t` +
    `syg_template_arg_t`. `src/stage0.hpp` = reflection layer:
    `generate_value<T>` / `generate_component<T>` mint a `syg_type` from a C++
    type (leaf/product recursion; GCC16 `-freflection`). Tests: `stage0_test`
    (green). Full design in **`stage0.md`** (thorough, incl. Open seams) +
    `agent_notes/type-theory.md`.
  - **PARKED (pre-registry sketches, CMake targets removed):** `src/variant.hpp`
    (sum type minted at runtime), `src/graph.hpp` (runtime graph as a syg_type) +
    their tests. They need the operator registry to hold their behavior; they
    return once it exists.
  - **Where the design landed:** behavior moved OFF `syg_type` into an **operator
    registry** keyed by `(name, endpoints)`, resolved at BUILD time; `syg_type` =
    pure data + one HOT `tick`; a `syg_value` is its own frame (members = args +
    return slots). `impl` = the type's SOURCE (graph topology / native source /
    `{}`). Deeper floor (horizon, in `stage0.md` "registry endgame"): a type's
    minimal descriptor is a **construction-term** → type universe = Merkle DAG of
    type-constructors; only `tick` is hot; grounding bottoms out in a pre-agreed
    **genesis vocabulary** + the senses.
  - **Next action (this thread):** draft the decree document (versioned,
    prose + roster table) for Travis to ratify; then the registry/table as a
    node over `syg_node_t` (content-addressed half + ref half), onto which the
    operator-registry plan (seam 0) and the parked variant/graph land. The
    `struct_` constructor forces variable-arity nodes — missing vocabulary,
    sketch next.

- **Next action (escapement thread):** open the crown's parked seams (kinds-by-index → registry
  organ; typed constants; bounds-as-fault, law.errors_are_values), OR the smallest playable
  graph driving one sense. Do NOT race `conformance/run.py`; it trails.
  NOTE: `src/escapement/` is the live tree; `src/syg/main.cpp` still dispatches
  nothing. Tests are in-tree beside their source (`X.hpp` ↔ `X.test.cpp`) and
  run via CMake/CTest: `cmake -B build -G Ninja && cmake --build build &&
  ctest --test-dir build` (4 tests, all green). The component test needs the
  reflection compiler; if CMake grabs GCC 15 (stale cache), nuke `build/` and
  reconfigure with `-DCMAKE_CXX_COMPILER="$(command -v g++)"`.

- **Active disciplines:** **50-line C++ cap** (never present more than 50
  lines of C++ at once — cross it and stop to explain; build/nix/cmake/shell/
  android exempt); read-and-sign (Travis's comprehension is the clock); gate on taste, not the suite; explaining IS the nativeness gate
  (a native that can't be justified out loud dies); sense-first build order;
  watch for hollow EXPLANATIONS (he predicts the next line / catches the code).

- **Open with Travis:** how to reform the conformance suite for this
  direction — invert it so human-blessed sense checkpoints lead and machine
  gates trail (his suggestion invited, not yet designed).
