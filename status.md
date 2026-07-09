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

- **Next action:** open the crown's parked seams (kinds-by-index → registry
  organ; typed constants; bounds-as-fault, L19), OR the smallest playable
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
