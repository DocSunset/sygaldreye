# Architecture Decision Record

*ADR-001…005 are PROBE-ERA platform choices (still true of the deprecated
design probe; open for revisit in the greenfield build, per the book's
appendix). ADR-006…028 are the current design.*

## ADR-001: Target platform is Meta Quest 3

Native Android (arm64), no game engine.

## ADR-002: XR runtime is OpenXR via Meta's OpenXR Mobile SDK

Vendor-neutral API; Meta's first-class supported path. Avoids proprietary lock-in while retaining full access to Meta extensions.

## ADR-003: Graphics API is OpenGL ES 3.2 / EGL

Chosen over Vulkan for lower setup complexity and faster iteration early in development. Meta's OpenXR samples have GLES reference paths. Revisit if CPU overhead becomes a bottleneck at 90/120 Hz frame budgets.

Graphics binding: `XR_KHR_opengl_es_enable`.

## ADR-004: Activity type is NativeActivity with native_app_glue

GameActivity (Android Game Development Kit) rejected — its advantages (richer input handling) are irrelevant when input comes through OpenXR action sets. `native_app_glue` ships with the NDK; zero extra dependency.

## ADR-005: Math library is Eigen

Header-only, no build step, well-suited for the linear algebra required (vectors, matrices, quaternions for transforms and poses).

---

*ADR-006 through ADR-012 ratified in the 2026-07-02 architecture session (Travis + Claude). Detail: planning/{bootloader,datasets,naming,trust,rhizome}.md.*

## ADR-006: Datasets and derivations are the foundation

The graph is not the fundamental object; a dataset is (bytes + kind + provenance, content-addressed). A graph is a dataset an executor can run. **A dataset is a node**: outputs are its data, inputs are links to other nodes' data; it never ticks. Two commit paths: **derivation** (provenance is a recipe — re-derivable, memoizable, evictable) and **capture** (provenance is testimony — irreplaceable; the sacred ground every derivation chain bottoms out in).

## ADR-007: Naming — root + route; mutability lives in the name

Two primitive name kinds: hashes (content-derived) and local names (container-scoped). An address = root + route. Fixed iff it traverses no ref (normalizable to a hash); live otherwise (a subscription). Route-names absorb edits; hashes deflect them into new nodes — so the store needs no special edit vocabulary. Provenance records hashes only. Formats: CID/multihash, Merkle-DAG chunking. (planning/naming.md)

## ADR-008: The store is a graph; availability is provide/compatible

A store is a graph of dataset nodes, reached by wiring (lexical, not ambient); plural, nested, scoped. get/put/ref/query dissolve into wiring/edits/named-nodes/traversal. Availability reuses the capability model: a peer **provides** a dataset (promise to serve, = pins) or is merely **compatible** (holds a copy, promises nothing). No distributed GC: durability = "someone provides it"; captures are provisional until provided; "archive" is a provide-everything policy peer. IPFS semantics and formats, mesh-local transport, Nix-style provenance on top.

## ADR-009: Trust — keypairs, advertisement, graphs vs plugins

Per-peer keypairs + pairing ceremony (not a shared PSK): revocation, unforgeable capture testimony, signable provenance. Advertisement is the permission system (placement is pull-shaped; selective advertisement is the sandbox). Graphs flow freely in-mesh; native plugins are a distinct act gated by provenance policy. At scale, membership grades into circles; only vocabulary and refs are ever gated. (planning/trust.md)

## ADR-010: Engine extension points are ports; order is wiring

A capability-package splice is a derivation over the engine graph, but the well-behaved package wires into published fan-ins (recognize-region, construct-context, choose-adapters) rather than rewriting. Pass order is declared by wiring (draw-chain precedent), visible as patch structure.

## ADR-011: Detachment — realized views are editing surfaces

Realized views write back through the compilation's inverse route map; compilation inserts default mappings only where none sits; conditional-on-compilation behavior is a (deliberate) pass; refusing write-back is a fork (a recorded ref rebind). No override/patch shadow dataset. A graph is a composite node (topology + defaults); presets are defaults nodes.

## ADR-012: Sygaldreye subsumes Rhizome

A rhizome entity is a dataset is a node; transclusion is an address-valued link; the "proposed program to render" is a declinable graph run in the reader's sandbox. The rhizome/astui project is an abandoned design probe; its durable context lives in planning/rhizome.md.

## ADR-013: Feedback is one sample by default; cycles are per-sample islands

Within a region, the default z⁻¹ delays ONE SAMPLE. The interpreter executes each cycle (strongly-connected island) sample-interleaved — per-sample kernels ticked once around the loop per sample (Pd `block~ 1`, scoped automatically to the island); feedforward stretches keep fused per-node block loops. Freezing fuses islands to loop-carried variables: a pure optimizer, identical semantics (upholds law.format_is_law "format is law"). Opting out is wiring: an explicit block-sized delay in the loop breaks the island. A cycle through a block-override node (FFT-shaped work) forces an explicit block delay at that edge, loudly, at edit time. Islands render visibly in the editor (cost transparency). Ratified 2026-07-03; enabled by the kernel-extraction discipline. Supersedes block-granular cycle delay.

## ADR-014: Freezing is a backend of realization

The engine pipeline's realize stage has two backends: interpret (instantiate natives + plan) and codegen (the freezer: emit a fused, portable, typed C++ class). Same passes, same compilation map, same recipe provenance; freeze = compile with `backend: codegen, target: <tier>`. This folds the freezer into engine-graph vocabulary — one compilation model, never a parallel system (the 2026-06-12 postponement's fear, dissolved). With ADR-013, the freezer is a pure optimizer. Unfreezing = reading provenance; stage 0 is ultimately the codegen backend applied to the boot graph. Ratified 2026-07-03.

## ADR-015: Execution semantics are rate-keyed (push, pull, clocked)

Event ports PUSH: must-not-drop, same-tick topological propagation in-region (appliers before process), queues across threads. Value ports (frame/control) are DIRTY-PUSH + DEMAND-PULL: writes propagate staleness; executors, probes, and derivations demand values; static subgraphs quiesce automatically (lazy but never stale). Stream ports run CLOCKED, always-hot, dirty-exempt (ADR-013 baseline retained). Executors own their region's discipline. CLOCKS ARE INPUTS: time-dependent nodes wire to their executor's published clock — "always dirty" is visible dataflow, not a node property (kernels already ban absolute time). Effects live in executors and sinks; a node with no demanded output and no clock input is provably inert (edit-time lint). Dissolves hot/cold inlet conventions and trigger-object sequencing into the type system and order-is-wiring; replaces tick-everything frame semantics. Ratified 2026-07-03 (greenfield).

## ADR-016: The fault model — errors are values; failure is death; handling is a package

Noexcept is the default node promise. Fallible nodes DECLARE a fault output; only their generated shells gain a catch (exceptions never cross the ABI); expected errors are VALUES on ordinary edges — whoever cares wires to them. Breaking the noexcept promise (undeclared throw, hardware trap) kills the CONTAINMENT UNIT (slot or process); a faulted producer severs and consumers fall back to their inlet defaults (no per-output bypass declarations — the unconnected-input mechanism, reused). Supervision = a parent noticing death; policy (restart ladders with intensity limits, severance, alerting) is WIRING — spawn-and-park is the frozen base case. Detection is overlay and demand-driven: NaN guards are mappings (package-default, removable), watchdogs subscribe to executor heartbeat/timing outputs (unmeasured unless demanded — ADR-015), underrun counters surface as outputs. Fresh plugins realize in subprocess regions (quarantine tier); promotion is trust policy (msh.graphs_vs_plugins) — containment and trust are one dial. Testimony buffer + death-watch are boot-graph nodes: hosted targets include them by default, freestanding freezes omit them. Fault handling is a capability package; nothing is imposed. Ratified 2026-07-04.

## ADR-017: Two-lane at-rest encoding; codecs are generated

Structured metadata (graphs, defaults, provenance, edit ops, kind nodes, ref values) encodes as IPLD **dag-cbor** (sorted keys, definite lengths, canonical floats, no NaN/Inf, links as first-class CID values); hashes are computed over these canonical bytes. Bulk payloads (audio, video, meshes, textures) are **raw-lane** bytes in kind-specific encodings, Merkle-chunked, hashed as-is (NaN legal — it's signal, not structure). **JSON is a projection**, tolerant by design: every human/agent surface speaks it; commit converts through the canonical encoder, so formatting can never fork identity. In-motion representation stays C/C++ structs; the translation is **generated from the single declaration** (the endpoints/schema struct, via the same reflection walk as aut.generated_descriptors): in-motion layout, at-rest codec, and JSON projection from one source — the generated encoder is the ONLY writer of at-rest bytes, and round-trip property tests are auto-generated per kind. Hash-and-encode happens only at commit boundaries (commit, advertise, memo-lookup), never on tick paths. Declaration changes change hashes = kind evolution (its own design item). Node-type references in graphs pin CIDs; whether via a per-graph vocabulary-lock object is decided with the perpetual-greenfield item. **Projections are open-ended**: the three above are merely the privileged minimum — every kind may carry arbitrarily many further projections (HTML page, VR view, Markdown, TUI, …) as ordinary decoder programs (datasets: shippable, sandbox-run, declinable). Exactly ONE projection is authoritative for identity (the canonical at-rest bytes); a projection that publishes an inverse map is an editing surface (cmp.projection_editing generalized) — this is the hypermedium/graphiverse mechanism. Ratified 2026-07-04.

## ADR-018: History is the op log

All mutation flows through structured edit ops (lng.edit_ops); the session's history IS the op log — no snapshot semantics anywhere. Ops carry their inverses (set_param records the prior value; remove_node records the removed record and edges): undo = apply inverse, redo = re-apply. GESTURES ARE TRANSACTIONS: gesture brackets coalesce ops on the same route, so a slider drag is one undoable unit — retires the structural-vs-param snapshot heuristic and makes param gestures undoable. Snapshots demote to semantically-inert checkpoints (fast time-travel over long logs). Commit folds the log into the new object; the ref hash-trail is the coarse history, the op log the fine one, and a session's log is itself committable (provenance for "how did this patch get this way"). Ops are attributable (author peer key) — groundwork for audit and multi-writer collaboration. THE LOG IS A TREE; LINEARITY IS A VIEW: append-only, undo moves a cursor, an edit after undo branches instead of discarding the redo tail; the default UI is linear undo/redo along the current branch; the tree view is just another projection (ADR-017). Refs already fork at coarse granularity — one history shape at two zoom levels. Cherry-pick = deterministic replay of an op range; MERGE semantics deferred to the multi-writer design (same problem, one answer). Ratified 2026-07-04.

## ADR-019: One core, many hosts — machinery language and Python/JS interop

Core machinery is C++ (the kernel contract and freezer emit/consume it; NDK + Emscripten first-class; PFR→static-reflection generation path). Because machinery is invisible behind links, per-machinery substitution (e.g. Rust for a network-facing piece) stays legal forever without ontology impact; safety now comes from generated-smallness + fuzzing + quarantine containment (ADR-016/017). **No protocol logic is ever written twice**: other languages either LINK the one core — `import sygaldreye` is a trampoline; a CPython extension boots the same peer in-process, as the Emscripten build already does for the browser — or speak the GENERATED wire codecs when genuinely remote. Bindings are generated projections of the single declarations (Python dataclasses, TS types) — bindings that cannot rot. Graph-executes-Python and Python-executes-graph are the same embedding: Python/JS author worker-region node types (behavior links to source datasets; in-process, the hosting interpreter IS the executor's machinery), and graphs are Python callables (derivation mode: inputs → run-to-completion → outputs, provenance recorded), with edit ops as the API and span/audio payloads crossing zero-copy via the buffer protocol. Real-time regions stay native; a Python "oscillator" is a prototyping skin re-kerneled or frozen on promotion. Ratified 2026-07-04.

## ADR-020: Three disciplines forever; clocks are open

The rate lattice is exactly three DISCIPLINES — event (push), value (dirty-push/demand-pull, cadence-free), stream (clocked, parametric over WHICH clock) — and they are semantic categories, not frequencies. Cadence is not part of the type system: a clock is an executor's published output (ADR-015), so new "rates" arrive as new executors in capability packages (video 24fps, IMU 100Hz, MIDI pulse — which is events, not a sample clock) with no core change and no global rate enum. Kinds may pin clocks (audio pins block) as kind-node constraints (lng.kind_catalog). Regions are connected components under one clock's demand; crossings between clocked domains are boundary mappings (resamplers, jitter) supplied through the owning package's choose-adapters fan-in — visible and replaceable, as always. Ratified 2026-07-04.

## ADR-021: Determinism classes

Every derivation declares one of four classes — one more member of the promise family. **exact**: bit-reproducible everywhere (structural/integer work; graph compilation; codegen text); memo shared mesh-wide; verification = re-derive and compare hashes. **platform-exact**: reproducible given a platform fingerprint (arch + toolchain-closure), which joins the memo key. **approximate**: same semantics, different bits (float pipelines, GPU, threaded reductions); memo is first-write-wins per key (trust the deriving peer's signature — msh.graphs_vs_plugins posture); provenance may hold a SET of outputs for one recipe; equality between them is kind-specific tolerance, never hash equality. **nondeterministic**: undeclared randomness / wall-clock / mid-derivation network — not memoizable, and honestly not a derivation: DECLARE THE INPUT OR IT'S A CAPTURE (seeded RNG is fine — the seed is an input); hidden nondeterminism is an honesty bug, lintable where detectable. Verification depth and eviction safety follow the class; derivation mode may opt into stricter float discipline (no fast-math, fixed thread order) to climb classes when reproducibility outranks speed. Ratified 2026-07-04.

## ADR-022: QoS-aware placement; predicted/posed edges

Requirements from types, offers from measurement, arithmetic at compile time, latency visible on the wire, prediction as a kind, policy as a pass. The mesh measures itself demand-driven: link RTT/jitter/bandwidth and peer load are net-package executor outputs (unmeasured unless demanded). An edge's discipline + clock IS its requirement (block crossings carry the block deadline; events need ordering; values tolerate staleness by pull construction); placement legality is first-order compile-time budget arithmetic — can some adapter satisfy this crossing on the measured link? Latency is REIFIED on net mappings (measured + declared, visible parameters); unsatisfiable placement fails loudly (fallthrough or unplaceable), never silent degradation. The placement policy (local > lowest-latency > least-loaded) is itself a replaceable pass. **Posed/predicted edges**: a kind whose value is a FUNCTION OF TIME (sample + timestamp + motion model), evaluated at the consumer's clock — extrapolation, not buffering (head pose → remote HRTF per block is the canonical case); prediction error is approximate-class semantics (ADR-021). Ratified 2026-07-04.

## ADR-023: Multi-writer — one live arbiter; history merges, not state

LIVE: every running instance has exactly one op queue (on its host peer); all writers — humans, agents, remote peers — submit attributed ops; ARRIVAL ORDER IS THE ORDER (game-server shape; the existing edit queue, formalized). Same-param collisions resolve by queue order (two hands, one knob); structural collisions are prevented by OP PRECONDITIONS (an op names what must exist; the loser fails loudly — lng.edit_ops addition). Presence is ordinary shared state. No optimistic-rollback machinery until lag is actually felt. APART: peers fork refs (provenance-or-fork) and grow op-tree branches (ADR-018); MERGE IS A DERIVATION — deterministic replay of an op range; precondition failures become a CONFLICT REPORT DATASET, resolved by authored resolution ops (agents welcome), the whole merge provenance-recorded. CRDTs are deliberately excluded from the core (live instruments need authority-now, not convergence-eventually; silent convergence = silent semantic surprise) but admissible per-kind behind links (e.g. collaborative text). The arbiter is supervised like everything else; the op log reconstructs the queue. Ratified 2026-07-04.

## ADR-024: Queries are graphs; the query vocabulary is core

A query is a graph wired from a small CORE vocabulary — traverse, filter, join, fixpoint (transitive closure; terminates by construction over finite stores) — part of the finely curated fundamental node set, because palette, search, lineage, back-links, and the store browser all stand on it. Run once = a derivation: the answer is an INDEX dataset with recipe provenance, memoized. Left running = a standing query: a reactive value whose cone commits dirty-push into (ADR-015) — incremental view maintenance for free. Indexes are datasets, hence providable; mesh queries fetch or fan out to providers' indexes; results are honest (bounded by reachable trust domain). sto.back_link_index's back-link index becomes an instance of this design, not a special case. A textual query syntax, if ever wanted, is a projection decoding to a query graph (ADR-017) — the core never grows a parser. Ratified 2026-07-04.

## ADR-025: Node succession (evolution is general, not kind-specific)

Hash-named nodes are SUCCEEDED, never mutated. Succession is implicit in every ref trail; it is DECLARED (a `supersedes` link on the successor, optionally carrying a migration) in proportion to fan-in — private data just versions; kinds and node types, which thousands of objects lean on, declare succession so dependents can follow. MIGRATIONS ARE DERIVATIONS attached to the successor: for plain data the migration program already exists (the op-range between versions — ADR-018); for kinds it's an authored (or generated-for-trivial-cases) program transforming dependent objects; structural migrations should be exact-class (ADR-021) so upgrades hash identically mesh-wide. Migration is LAZY (migrate-on-read, memoized, provenance-recorded); nothing is ever bulk-rewritten or forced. Compatibility and upgradability are REACHABILITY QUERIES over supersedes/migrates-from links (ADR-024). Runtime migration (state across swap) is the live face of the same concept: applying a succession to a running instance, state carried through the map. Old data, old graphs, old vocabulary remain valid forever under their original hashes. Ratified 2026-07-04.

## ADR-026: Perpetual greenfield — locks, conformance, ratchet

Three commitments that keep the project greenfield by construction. (1) **Vocabulary locks**: a graph's topology refers to node types through a per-graph lock object (name → type CID), content-addressed — perfect pinning, but upgrading a patch is one link swap and a vocabulary migration rewrites one small object, never every topology. (2) **The conformance suite IS the system**: the book's acceptance criteria, executable, define sygaldreye — an implementation is sygaldreye iff it passes; total reimplementation is a normal, checkable act. (3) **The core ratchet**: the core only shrinks — new functionality must be a package or a dissolution into existing vocabulary; additions to the core require demonstrating no composition can express it (the vision's measuring stick, promoted to law). Mechanism summary: maintenance = regeneration, change = succession, implementation = cache, and the core cannot accrete. Ratified 2026-07-04.

## ADR-027: The self-hosting closure

The perpetual-greenfield properties apply to the core and to the definition of conformance itself. **The core is an artifact of the system** (ADR-014 assembled): native sources are datasets, the boot graph is a dataset, the toolchain is a Nix derivation, the core binary is a derivation with recipe provenance in the store — editing the core happens through the system's own documents and ops; sygaldreye-N derives sygaldreye-N+1 (freeze-as-a-service pointed at ourselves). **The conformance suite is written in the system**: a test is a derivation (test graph + blessed expected outputs + comparison program), with comparison semantics given per-test by determinism class (ADR-021 — exact: hash equality; approximate: tolerance graphs); the harness boots the candidate AS A PEER and exercises it over the mesh protocol — conformance testing and interop testing are the same act. The suite evolves by succession, provenance-tracked, and any conformant implementation can judge any candidate. **The fiat floor** (all that never becomes trivial): the header convention (frozen by design), the trampolines, the toolchain/physics boundary, the blessed outputs (testimony), and PRE-EXISTENCE — one conformant realization must keep existing. Trusting-trust is answered structurally: conformance is behavioral and judged by other peers; multi-implementation re-derivation is diverse double-compilation. Ratified 2026-07-04.

## ADR-028: The escapement, the movement, the crown, the complications

The core, ratcheted to its floor, in horological nomenclature. **The escapement** — the node contract + tick-in-order: the only unconditional substrate; a calling convention and a for-loop, constexpr-able. **A movement** — a frozen, flattened, realized graph the escapement ticks: a sealed firmware is escapement + movement, provenance carried for unfreezing. **The crown** — the minimal self-modification machinery, three parts: the plan as mutable data; ONE applier primitive executing edit ops against it (instantiate-linked-native, link, unlink, remove, write-default), applied at tick boundaries (atomicity free at control rate); and an op inlet. A graph ≡ the ops that build it (ADR-018), so the boot "graph" is a BOOT TAPE of op records replayed through the crown — no parser, codec, or resolver needed at rung one; stage 0 refines to escapement + crown + linked natives + tape. The full slot (batched, transactional, state-migrating) is the crown's grown-up form. **Complications** — everything else, INCLUDING the liveness organs (parser, codec, naive resolver, registry-face, slot, subgraph, ref, reflection seam, the query four, the seven canonical mapping definitions): core-NAMED where they pass the no-composition test, but nodes like all nodes, present only by tape/boot-graph choice. Presence is always chosen; nothing is imposed; a sealed movement has no crown, like a watch with a welded case. Conformance splits into two profiles: movement-level (behavioral — golden outputs, provenance) and peer-level (protocol — pairing, ops, fetch, advertisement). The ladder of being: a movement does one thing; + crown, it can be changed; + complications, it can become anything — data flowing through graphs all the way down to the trampoline sitting on the ground. Ratified 2026-07-04.

## ADR-029: Addresses are routes from here (no root primitive; no ref registry)

Amends ADR-007. **An address is a route, walked from HERE** — the resolver's ENVIRONMENT node: its wired stores, object store, peer table, petname table. This is astui's ground, per-resolver, never global: each resolver stands on its own. Every former "root" is just a first step answered by an environment container: unqualified ref-names resolve LEXICALLY against the wired store environment (like a relative path against cwd — the store-as-graph ambient-authority kill, completed); hashes are content-derived names answered by the object store, their referents pinned by re-hash verification regardless of container; fully-qualified refs are PEER-KEY steps — self-certifying (the name carries its verification material), ref-states travel SIGNED, each peer is sole authority for its own refs (single-writer), discovery is advertisement, never registration (mDNS/static/DHT per scale — the msh.discovery seam); PETNAMES humanize keys locally (a private store graph, no pretended global truth). Liveness restates per-step: a step is fixed iff its name is content-derived or conferred by immutable containment; an address is fixed iff every step is; refs are the conferred-mutable steps. The `root:route` spelling survives as serialization sugar. Ratified 2026-07-05.

## ADR-030: "Type" dissolves — port promises are kind + discipline links; a node type is a kind with behavior

The type bundle (`#t-… = {kind, rate}`) is retired: rate was already a link property (ADR-015/020), so the bundle had no glue. A port declaration attaches its promises DIRECTLY as links (`ports/freq/kind → scalar`, `ports/freq/discipline → value`); the first-order oracle reads (kind, discipline) pairs; nothing behavioral changes. The deeper half: an instance's `type` link and a datum's `kind` link are the SAME move — classification conferred one level up by linking context — so the one-kind-system ratification extends over instances: **a node type is a kind that carries behavior** (decoders + behavior link + port declarations; a data kind is the behaviorless case). "Node type" stays ratified as the idiom for behavior-carrying kinds; bare "type" moves to retired prose. Ratified 2026-07-05.

## ADR-031: Derivation is the shape; commitment is the act

A DERIVATION is any application of a program to inputs yielding outputs — the general computation shape. A tick is a derivation in time; an instantiation is a derivation in space; both uncommitted. A COMMITTED DERIVATION is one whose recipe is recorded: provenance written, memoizable, determinism class declared (classes attach AT COMMITMENT — an uncommitted derivation promises nothing and needs no class). "Derivation mode" survives as the executor idiom, precisely glossed: run to completion AND commit — deriving with the recorder on. Corrects the glossary's earlier "a derivation is a committed computation," which re-welded what the two-doors design deliberately split. Ratified 2026-07-05.

## ADR-032: Versions are derived — semver as a verified projection of succession

Every declared succession (ADR-025) carries a CLASS — breaking | additive | fix — semver's three digits minus the numerology. Version numbers are DERIVED, never assigned: `name@2.1.3` = two breaking hops from the chain's origin, one additive since the last breaking, three fixes since the last additive, computed by a fixpoint query over supersedes links (lng.query_vocabulary); numbers are stored nowhere and so can never disagree with history. Classes are VERIFIED, not vowed: additive/fix successions must keep the predecessor's conformance criteria green and remain decode-compatible; breaking successions carry a migration or explicitly declare none — semver's Hyrum-law failure mode becomes a checkable, checked claim. Nothing load-bearing changes: identity stays hashes; mesh compatibility stays reachability; `name@M.m.p` is ref-sugar resolving through the chain; semver is a projection (ADR-017). Applies everywhere: kinds, node types, spec nodes (ABI contract, format pins), the book's own editions, and plain git tags for repo releases. Ratified 2026-07-05.

## ADR-033: The native metric — machinery, the per-sample floor, or maturity

The default is GRAPH (a graph line is live-editable, provenance-tracked, placeable, freezable; a native line is opaque and toolchain-bound). A node earns nativeness through exactly one of three declared clauses: (1) MACHINERY — it touches the world (device, OS, thread, syscall, interpreter, trap); its interior couldn't be links; (2) THE PER-SAMPLE FLOOR — kernels (aut.kernel_contract); (3) MATURITY IMPORT — wrapping a tested implementation (you're buying its test suite, not its line count). Corollaries: WHEN A GRAPH IS TOO SLOW, FREEZE IT — after the freezer exists, performance alone never justifies a native; TRIVIAL NATIVES ARE GENERATED (one-line kernel lambdas; the aut.generated_descriptors stamp makes leaf arithmetic cost nothing); every native declares its clause (gate-checked); scaffolding natives are marked and dissolve. The NATIVE LEDGER (ch. 16) records the expected set (~70 at maturity, half generated one-liners) — and is EXPECTED TO CHURN as implementation teaches: ledger changes are ordinary reviewed commits, not ADRs; only this metric is fixed. Ratified 2026-07-05.

## ADR-034: Structured payloads and the realization rule — graphs run through the one contract

Closes the gap the first rung-7 pass exposed (the "hollow engine": engine-v0 shipped as data but walked by bespoke C++ — its passes never instantiated, never ticked, `pass` not even a registered type; the same pattern as the query evaluator). Two clauses.

(1) **Structured payloads arrive by contract succession** (ADR-025). The node contract gains a kind-tagged structured lane: ports may carry graph, edit-op, text, and the catalog's other non-float kinds, declared in the same endpoints structs as every port, with codecs and accessors GENERATED (abi.one_declaration/aut.generated_descriptors) and legality decided by the same first-order oracle over (kind, discipline). In-process hops are zero-copy; hashing and canonical encoding happen only at commit boundaries (ADR-017), as ever. The float/stream path is untouched — structured payloads ride the event and value disciplines only; the oracle refuses them on stream — so the RT audits and golden audio cannot move. This is the substrate lng.text_events was waiting on: a graph edits a graph by wiring an op-kind event into an arbiter inlet (lng.structured_payloads.op_event_applies); lng.text_events's remaining open question is text semantics only.

(2) **The realization rule.** Anything the book calls a graph — the engine pipeline, queries, policy, the editor — is REALIZED: its node types registered, instantiated into a plan, ticked through the crown/plan machinery, exactly like hello-cosine. A bespoke evaluator that walks graph-shaped data and dispatches on type strings outside plan realization is SCAFFOLDING (ADR-033): its clause marker must name the criterion that dissolves it (`// clause: scaffolding (dissolves: cmp.engine_is_realized.pass_edit_changes_compile)`), the gate checks the marker, and it is a standing work item, never an endpoint. Retro-marked instances: the compiler's engine walk (dissolves: cmp.engine_is_realized.pass_edit_changes_compile) and the query evaluator (dissolves: lng.structured_payloads.query_evaluator_dissolves). Rationale: a graph interpreted by a private evaluator has no ports, no palette entry, no live editability, no placement — it is data wearing the costume of the medium; need.liveness/need.uniformity are satisfied only by realization. Requirements: cmp.engine_is_realized and lng.structured_payloads (both rung 7); cmp.extension_ports.additive_splice/3.2 and cmp.laziness.zero_resident_engine strengthened to observe the realized plan rather than trust the compiler's self-report. Ratified 2026-07-05.

## ADR-035: The mesh crypto suite — boring primitives, pinned once (DRAFT — Travis to ratify)

The design (ch. 8) says peer identity is a keypair, pairing records key acceptance, testimony's peer-id IS a public key, and "transport authentication/encryption falls out." Ch. 14 pins the wire *table* (varint kind + dag-cbor body) but never the *primitives*. This ADR fills that gap with the boring, standard choice the scale-posture already names ("the Web's and the package manager's trust models"):

- **Identity + signatures + testimony + provenance:** Ed25519 (libsodium `crypto_sign`). A peer-key is the 32-byte public key, text-encoded as the address grammar's `#peerkey` self-certifying step (multibase base32, the FMT pin) — the same encoding CIDs use, so no new codec. Signatures are deterministic given the key; seeded keys make tests exact-class.
- **Pairing + session establishment:** each peer holds a long-term Ed25519 identity key. A connection is a handshake: exchange identity public keys + ephemeral X25519 key-exchange public keys (libsodium `crypto_kx`), each side signs its ephemeral key with its identity key. A peer accepts the handshake iff (a) the signature verifies AND (b) the remote identity key is in its accepted-keys set (paired). Revocation removes the key from that set and drops live sessions. Unpaired ⇒ handshake refused ⇒ socket closed; there is no pre-auth service surface (msh.authenticated_transport).
- **Authenticated encryption of the channel:** libsodium `crypto_secretstream` (XChaCha20-Poly1305) keyed by the `crypto_kx` session keys; every framed message (varint kind + dag-cbor body) is one secretstream message. Authenticated AND encrypted, per msh.authenticated_transport.
- **Transport:** TCP (loopback in-household; the WebSocket framing the browser needs is the same length-delimited records over a WS carrier — deferred until the browser peer, rung 10). Discovery is an abstract provider (msh.discovery): a static peer list or an mDNS beacon, semantics-identical.

Rationale: nothing exotic, nothing hand-rolled — libsodium is the maturity import (ADR-033 clause 3). The mesh transport/crypto is MACHINERY (clause 1: sockets, syscalls). Reachability, not equality, still governs compatibility; identity stays public keys; secrecy-as-decoder-scarcity (ch. 8) is unaffected. Pinned as an fmt.pins_frozen succession point: changing a primitive is a succession of this spec, never an edit. Drafted 2026-07-05 by the builder; awaiting Travis.

## ADR-036: A `fixture` clause for permanent test fixtures and provocateurs (DRAFT — Travis to ratify)

ADR-033's native-clause taxonomy (machinery | floor | maturity | scaffolding) has no category for a node whose ENTIRE PURPOSE is to exercise the machinery from the test side — an audit node that deliberately violates a contract (an RT-allocator that proves the no-alloc audit bites), a fault-matrix provocateur (nan_bomb, spin, sleeper), or an ABI demonstration struct (widget_a/widget_b, the "one declaration line adds a port" pair). These are PERMANENT — a deliberate-violation probe can never be a well-behaved real native, because a real native never commits the violation — yet they are not scaffolding (nothing dissolves them), not the per-sample floor (many have no kernel at all), not machinery in the world-touching sense, and not a maturity import. The rung-12 audit caught three re-aims stretching `floor`/`machinery` to cover exactly this gap.

Add a fifth clause: **`fixture`** — a node that exists solely to exercise or demonstrate the system's own contracts from the conformance side; permanent by nature, never shipped in a movement, and correct precisely because it does the thing a real native must not. Examples: `fault_natives` (tcf.fault_matrix provocateurs), `testnodes` (abi.hook_discipline/3 audit bodies), `widgets` (the abi.one_declaration.field_surfaces_port declaration pair). The clause gate (cor.native_ledger_discipline.clause_marker_required) accepts it as a fifth valid marker; the dissolution gate is unaffected (a `fixture` names no criterion). Nothing else changes: fixtures are still gate-checked to carry a clause on line 1, still live under `src/nodes`, and are never part of the shipped core (the native ledger's ~70 count excludes them, as it already excludes test scaffolding). Drafted 2026-07-06 by the builder after the rung-12 audit; awaiting Travis.

## ADR-037: The view is an edge (DRAFT — Travis to ratify)

The render/xr head takes `view_pose` (position + orientation; catalog kinds
vec3 + quat) as an ordinary INPUT port, default-wired from the `head_pose`
source node. Machinery composes per-eye views by expressing the runtime's
head-to-eye transforms (xrLocateViews) relative to the incoming pose; eye
projection matrices stay machinery. Rationale: need.agency (humans and agents share
paths) and law.graphs_first — where the camera sits is authoring, not machinery.
"Eyeballs in your hand" must be an edit, not a hack: rewiring `view_pose`
from head_pose to a controller pose is one live graph edit, undoable like
any other (pkg.xr_package.view_is_edge). On the host the same port feeds the window's view —
the shell and the headset differ only in which sources exist. Drafted
2026-07-06 by the builder alongside planning/embodiment_plan.md Phase 0;
awaiting Travis.

## ADR-038: Reification is the variable; the binding is reified, the instance emerges (DRAFT — Travis to ratify)

The level cut (declaration versus binding) is not two kinds of thing — it is
one node seen in two roles, and what differs is only whether, and how, a role
is made concrete. Ratified at stratum 0 by the escapement's own structs:

- **node = declaration, universally.** A node is a bag of named links, and a
  bag of named links already *is* a declaration of what links exist. This is
  true of a node type (declaring ports) AND an instance (declaring its bound
  values, `freq → 220`) — so "node = declaration" does not narrow law.one_form; every
  node is declarative. The escapement's `node` struct (arity + sizes +
  behavior) is ONE reification of a node in its declaration role, not the
  definition of node.
- **binding = the binding role, reified.** The escapement's `binding` struct
  (a word closed over pointers to its input cells and its output cell) is the
  binding side of the level cut made concrete. It is literally a closure that
  captures BY REFERENCE into shared state — an open closure / a bound
  application waiting to fire. This one fact grounds three earlier decisions:
  inputs must be const (you may not mutate a capture others share), fan-out
  works (many bindings close over the same cell), and the instance can be
  emergent (overlapping captures ARE shared fields).
- **instance = the node deliberately left un-reified.** There is no instance
  struct. An instance exists only by reference (law.existence_is_reference) — it is a *reading* of
  the state array against the movement (state × movement), a derivation
  produced on demand, never a stored datum. "The instance owns its state
  struct" (ch. 15) is one higher-stratum reification of this same emergent
  thing; at stratum 0 even that dissolves into flat state plus the bindings
  that close over it.

Consequence for the book: the glossary's "declaration versus binding" and
"instance" entries carry this note; **binding** joins the greenfield-stratum
vocabulary as the reified level-cut binding (the movement's rows). No law
changes — this sharpens law.one_form and law.existence_is_reference, it does not amend them. Drafted
2026-07-07 by the builder alongside the escapement build; awaiting Travis.

## ADR-039-identifier-slugs: identifiers are name slugs; ADRs keep their number and gain one

The book's needs, laws, requirements, and acceptance criteria are re-keyed
from ordinal ids (`NAM-1`, `L4`, `N8`, `EXE-5.1`) to descriptive, namespaced
slugs (`nam.addresses`, `law.provenance_or_fork`, `need.safety`,
`exe.migration.phase_continuous`) — dotted, domain-name-style, the family
surviving as namespace. The number carried no meaning; the slug does, and it
survives insertion and reordering where a number never could. Consequences:

- **The slug is the identity.** There is no parallel number to fall back on;
  a citation is the slug. The family prefix (`nam`, `exe`, `law`, `need`) is a
  namespace, not a sort key — collisions across families are harmless
  (`cor.two_profiles` and `cnf.two_profiles` coexist).
- **Ranges are retired.** `STO-1..9` had meaning only under ordinals; it
  expands to the explicit list of slugs. No compressed id forms remain.
- **The generator follows.** `conformance/extract_manifest.py` parses the new
  heading form (`**nam.addresses**`, `- nam.addresses.slug:`); the manifest,
  the `TESTS` maps, and the dissolution gate key on slugs. `run.py` is
  unchanged in logic — it never read the ordinal.
- **ADRs are exempt from re-keying but not from slugs.** Existing ADRs keep
  their bare numbers as historical record; their *body citations* were updated
  to the new slugs so cross-references resolve, but no decision, number, or
  structure changed. From this ADR forward every ADR's identity is
  `ADR-NNN-slug` — the number preserving ratification order, the slug an
  inextricable part of the name. Citing the number alone is henceforth an
  incomplete reference. This record is the first to carry one.

Ratified 2026-07-09 by Travis; applied by the builder in the same commit
(book, `adr.md` citations, conformance tooling). Governs Chapter 10 §VI
(amendment) and the lexicon's one-name-per-concept rule.
