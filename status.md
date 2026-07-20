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
  - **Landed (`11523a0`): `src/types.hpp` + test — decree-v1 constructors.**
    `GROUND` (zero digest) terminates type chains + doubles as anonymous;
    `fiat(name)` mints roster rows via the SAME preimage (one id scheme,
    recomputable): ATOM, STRING, STRUCTURE, VARIANT, MUTABLE_PTR,
    CONSTANT_PTR. Terms hold ONLY the underivable (atom = {name-id, size,
    align}; structure/variant share one [name][fields…] layout, split by
    constructor id; pointers = pointee id — ACCESS not ownership; arity rides
    in header size, unstored). Names are ids of STRING nodes ("every hash
    decodes"). All in `namespace syg`.
  - **Landed since (`b8b7a80`…`f90b159`):** node = (type, bytes) CONTENT; the
    one struct is `syg_handle_t {id, type, data, size, env}` (id = derived
    name, size = store fact, env = resolution context) — node.hpp. Preimage
    SHORTENED: `id = fnv1a(type ∥ bytes)` (size dropped — one terminal
    variable field is unambiguous). DYNAMIC = ~0 marks unsized (0 stays legal:
    type_tag); STRING demoted to a minted unsized atom named by fiat("string")
    (its own name can't be STRING-typed — hash fixed point; content-addressing
    makes reference cycles unconstructable, type chains ground at GROUND).
    `registry.hpp`: syg_registry_t {parent, table} — environments compose;
    insert_or_get copies + stamps env, confluent; floor() pre-registers the
    decree; atom/scope char* conveniences register names. `scope` constructor:
    qualification-as-content (Merkle name chains; name slot polymorphic
    STRING/SCOPE/GROUND by type; identity vs resolution split). Demotion
    ratchet: a roster row stays fiat iff its content can't be said from
    inside. types_test + registry_test green.
  - **Landed (`a67f5c5`): the ENVIRONMENT INVERSION — `src/env.hpp` (was
    registry.hpp).** `syg_env_t` = a lexical frame of grips (parent +
    symbols); HERE is the only out-of-band handout (ADR-029). Stores are
    ORGANS gripped at decreed names (CONTENT, REFS) — dumb maps; ONLY the
    environment composes (frame walks). Three contracts, three verb sets:
    wire/unwire/find (SITUATED — local, mortal, never ships; a symbol grip's
    id IS its name), insert_or_get/get (FIXED — verified dedup: byte-compare
    on id match, throw on mismatch ⇒ silent merge impossible), bind/follow
    (LIVE — `address` = content|symbol tag, so refs point at immutable
    content OR live grips; symbol-refs are local-only). Typed keys
    content_id/ref_name/sym_name. Instances/mutable data live in PLACED
    memory (arenas), never in stores; fn pointers are grips (never content —
    location not meaning; wasm fn "ptrs" are table indices). derived(rel,
    subj) = attach metadata without touching bytes (mesh-boundary note in
    env.hpp: exact-pair keys or crypto hash before foreign content drives
    local bindings). env_test green.
  - **Landed (`97203e9` + review sweep `61fec61`): nodes are BORN registered.**
    types.hpp: `inscribe_symbol` = the SOLE unregistered factory (decree
    precedes environments); constexpr roster constants, literals spelled
    once, iterable ROSTER (10 rows incl. CONSTRUCT). hash.hpp: `chars()` THE
    fold, str/bytes adapters, constexpr `syg_id` twin — preimage agreement BY
    CONSTRUCTION. env.hpp: all constructors funnel through `node()` →
    insert_or_get (stack terms, store's copy durable); typed key wrappers
    DROPPED (the verb says how a hash reads); string/u64/word types minted
    on demand (idempotent, literal once); SYMBOL/STRING split (a name ≠
    text); `word = (env, argc, argv)` = the ONE behavior ABI;
    `method_node` (grip on a word cell, id EMPTY: locations get names);
    `bind_method(relation, type, method)` = the one binding pattern
    (CONSTRUCT now; TICK/PLACE/ERASE verbatim later); `emplace_or_get` =
    mint by type id = tick the constructor once (THE tape op). OWNERSHIP
    LAW: handles never own; frames never own; tables own (symbol-table
    destruction-ownership designed = unwire ticks ERASE word by grip type —
    NOT yet built, floor organs immortal). Data-first handle (first-member
    pun). All tests green.
  - **Landed (`10d5cb7`, `87c3566`): ONE word ABI + the dispatcher.**
    `word = void(*)(void**)` — IDENTICAL to the escapement's (frames
    read-only structure, outputs write THROUGH slots, arity = signature
    knowledge via count endpoints, HERE is data — no env/argc/handles in the
    ABI). A method grip's TYPE IS ITS SIGNATURE (anonymous structure node,
    (name,type) fields, inputs-then-outputs — the shape reflection reads off
    C++; "the type of a behavior is how to call it" = the arrow, quietly).
    `resolve` (cold, cacheable — a binding IS a resolved method; seam-0
    query engine slots behind it) / `call` (the ONE marshaller: hash64 field
    ⇒ &arg.id, envptr ⇒ &env, rest_hash64 ⇒ remaining args as refs, last
    field ⇒ &out, else payload) / `dispatch` (any relation — a TAPE RECORD
    is (relation, subject, arg-ids…); the crown's applier is dispatch in a
    loop). `emplace_or_get` = dispatch(CONSTRUCT,…). Constructor words =
    generated-SHAPE raw-cell shims (hand-written promissory notes). New
    residents: hash64_fnv1a, envptr, handle, rest_hash64 (zero-size marker).
  - **Landed (`a5a56f3`): ONE TABLE.** The environment = parent + one
    pair-keyed table {context, designator} → handle (context = GROUND |
    scope | relation). A row is a GRIP (data→live memory, never ships) or a
    REFERENCE (type REF, data null, target in .id — handle-as-promise, pure
    value, ships; type column = customs officer). find() derefs REF rows
    (one level, from HERE); peek() = raw row; bind = wire-a-reference.
    DELETED: ref_store, address, follow, derived(), REFS fiat row (roster 9),
    bind_method's double write (ONE row now). REPEALED: "a grip's id IS its
    name" — .id everywhere = what-this-refers-to. symbol_id() constexpr in
    types.hpp; ref_type_id() env-free. Ch.2 live semantics (signed refs,
    subscriptions) attach to REF rows later. Exact-pair keys: collision-
    alias impossible; coordinate queries = filters (query-four substrate).
  - **Landed (`fba630a`): the CANON, TYPE-less.** canon_row = AUTHORED twin
    of atom_term (spelling for name-id; term() = the bridge); rows via
    make_canon<T> (canon_name<T> category+width — char→u8 decree, aliases
    collapse, wide/long-double refused; size/align from the compiler; a
    static_assert pins the conforming platform). Short names: nil, b8,
    i8..i64, u8..u64, f32, f64, str, ref, hash64_fnv1a (long on purpose —
    the name IS the algorithm). canon_type RE-DERIVES from the decree
    (mint-or-get; NOT a lookup — TYPE rows shelved until a spelling-
    without-definition consumer exists: THE TAPE; the earlier TYPE-relation
    detour was Travis's lookup instinct aimed at the wrong case — for canon,
    re-derivation is the native idiom). CONSTRUCT demoted fiat→decreed
    symbol (constexpr symbol_id; sharpened criterion: fiat iff UNSAYABLE or
    PRECEDES-THE-STORE). Roster: 8, growth per relation ended. envptr/
    handle/rest_hash64 deliberately non-canon (situated; open: canon status
    if signatures ever ship). Parked: syg_id demotion-to-internal.
  - **Landed (`9e374d9`): names ARE strings; SYMBOL retired.** Travis's
    move: only the string type's own name is fiat (STR_NAME, the fixed
    point — str can't name itself with a string); every other name is an
    ordinary string node, so the out-of-band surface shrank from
    every-name to ONE node. STR_TYPE constexpr via chained LSB-first
    mixes == runtime struct hash (conformance asserted: little-endian +
    packed atom_term). symbol_id/symbol_node → name_id/string_node;
    canon_row::term() branches ONCE at the knot; canon_type self-
    guarantees str residency before minting spellings. Roster still 8
    (SYMBOL out, STR_NAME in). "Is a name" now comes from POSITION
    (term slot, key coordinate), not the type column. Every id in the
    universe forked — fine, decree is draft; this closes before freeze.
  - **CANONICAL NAMES discussion (in flight):** settled so far — no
    automatic canon-naming machinery; naming = decree extension (agree
    spellings, re-derive; names are REF-row bindings, not identity).
    Qualified lookup = the SCOPE preimage fold, constexpr-able. Table
    key choice: {parent-scope, leaf} (grip_key native, enumerable) over
    {GROUND, full-chain}. Behaviors stay relation-addressed (no
    "::atom.constructor" names). Appropriateness criteria: subkey ids
    must have RESIDENT preimages (hard, at declare); subkey types
    should answer PRINT — a decreed relation, audited not gated
    (bootstrap: organ wires before methods exist). print derives
    spellings recursively (never stored ⇒ can't drift); parse is the
    tape's separate discussion. Wanted verb: declare (throw-on-rebind
    HERE; shadowing = child frame). Comparative frame Travis liked:
    we're a triple store w/ ONE index over an immutable heap; second
    index (per-context row list, FS-dirent co-location) only when a
    profile demands.
  - **Ratified (conversation): functions are scoped names; methods are
    convention.** A function = (name-in-scope, signature, transition);
    conceptual term {name, sig, body} with body=GROUND for natives
    ("decoder out of band" = you must have it linked) — NOT yet landed
    as a term type. Method = function whose scope is a type; TICK =
    just the function named "tick" in the type's scope. Overload
    contract (Travis's exact words matter): {type, name} row = THE
    DEFAULT method, resolved by name ONLY, no overloads at that level;
    beside it, rows at {scope{type,name}.id, signature} = exact-match
    overloads; WHICH path to resolve is the ASKER's choice. A default
    that internally re-dispatches to same-name overloads is one
    author's option, not the contract. At most one rest_handles
    binding per name.
  - **Landed (`819cc39`): rest_handles; rest_hash64 retired.** The
    decreed variadic default signature is {env, args: rest_handles,
    out: handle}; the marshaller emits TWO slots for a rest field —
    synthesized argc (never a minted node) + the caller's contiguous
    handle array (zero copies; idless grips ride through — the seam
    TICK will use; id-array was rejected: it can only spell the frozen
    world). structure_construct_word migrated (explicit count arg
    died); situated trio now envptr/handle/rest_handles.
  - **Landed (`05ec207`): rekey + rename.** Function rows key
    {type, name} (context = subject type, designator = name — scope-as-
    type); resolve/dispatch mirror the key order (subject, name); tape
    record = (subject, name, args…). method → function everywhere
    (struct function, function_node, bind_function); "method" survives
    only as prose for functions-scoped-by-a-type.
  - **Landed (`98157fd`): word atom + resident function structure.**
    "word" atom = the 8-byte fn-pointer cell (situated); function =
    named structure {code: word, signature: hash64}, C++ mirror struct
    function{word fn; syg_hash sig;} layout-asserted — the shape
    reflection will emit function_type FROM. Rows grip function
    instances, type column = function_type (honest customs; the old
    type-is-signature pun retired). call() fetches the sig by one
    get(). resolve casts UNCHECKED (const env can't mint the check
    type; constexpr function-type-id twin deferred to declare era).
  - **After the reflection probe/revisit: freshen `.claude/skills/cpp26`**
    with what we learn (known gaps found 2026-07-19: access_context arg on
    members_of; our GCC16 landmines unrecorded; ctor/dtor discovery;
    parent_of; offset_of/layout; overload-set wall; bases_of/P3293;
    define_aggregate).
  - **Then: REVISIT REFLECTION** — registration TU (describe_function emits
    words + signatures off C++, deletes hand shims/floor sigs; scoped ids;
    register_cpp<X>(env); component = type + CONSTRUCT/ERASE/TICK methods —
    ERASE feeds table destruction-ownership); re-aim generate_component to
    resident nodes (retire syg.hpp's old world). Then: TICK + smallest
    ticking graph; decree doc. Parked: fiat+env, string_node borrow,
    syg_id demotion.

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
