# Architecture Decision Record

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

Within a region, the default z⁻¹ delays ONE SAMPLE. The interpreter executes each cycle (strongly-connected island) sample-interleaved — per-sample kernels ticked once around the loop per sample (Pd `block~ 1`, scoped automatically to the island); feedforward stretches keep fused per-node block loops. Freezing fuses islands to loop-carried variables: a pure optimizer, identical semantics (upholds L7 "format is law"). Opting out is wiring: an explicit block-sized delay in the loop breaks the island. A cycle through a block-override node (FFT-shaped work) forces an explicit block delay at that edge, loudly, at edit time. Islands render visibly in the editor (cost transparency). Ratified 2026-07-03; enabled by the kernel-extraction discipline. Supersedes block-granular cycle delay.

## ADR-014: Freezing is a backend of realization

The engine pipeline's realize stage has two backends: interpret (instantiate natives + plan) and codegen (the freezer: emit a fused, portable, typed C++ class). Same passes, same compilation map, same recipe provenance; freeze = compile with `backend: codegen, target: <tier>`. This folds the freezer into engine-graph vocabulary — one compilation model, never a parallel system (the 2026-06-12 postponement's fear, dissolved). With ADR-013, the freezer is a pure optimizer. Unfreezing = reading provenance; stage 0 is ultimately the codegen backend applied to the boot graph. Ratified 2026-07-03.

## ADR-015: Execution semantics are rate-keyed (push, pull, clocked)

Event ports PUSH: must-not-drop, same-tick topological propagation in-region (appliers before process), queues across threads. Value ports (frame/control) are DIRTY-PUSH + DEMAND-PULL: writes propagate staleness; executors, probes, and derivations demand values; static subgraphs quiesce automatically (lazy but never stale). Stream ports run CLOCKED, always-hot, dirty-exempt (ADR-013 baseline retained). Executors own their region's discipline. CLOCKS ARE INPUTS: time-dependent nodes wire to their executor's published clock — "always dirty" is visible dataflow, not a node property (kernels already ban absolute time). Effects live in executors and sinks; a node with no demanded output and no clock input is provably inert (edit-time lint). Dissolves hot/cold inlet conventions and trigger-object sequencing into the type system and order-is-wiring; replaces tick-everything frame semantics. Ratified 2026-07-03 (greenfield).

## ADR-016: The fault model — errors are values; failure is death; handling is a package

Noexcept is the default node promise. Fallible nodes DECLARE a fault output; only their generated shells gain a catch (exceptions never cross the ABI); expected errors are VALUES on ordinary edges — whoever cares wires to them. Breaking the noexcept promise (undeclared throw, hardware trap) kills the CONTAINMENT UNIT (slot or process); a faulted producer severs and consumers fall back to their inlet defaults (no per-output bypass declarations — the unconnected-input mechanism, reused). Supervision = a parent noticing death; policy (restart ladders with intensity limits, severance, alerting) is WIRING — spawn-and-park is the frozen base case. Detection is overlay and demand-driven: NaN guards are mappings (package-default, removable), watchdogs subscribe to executor heartbeat/timing outputs (unmeasured unless demanded — ADR-015), underrun counters surface as outputs. Fresh plugins realize in subprocess regions (quarantine tier); promotion is trust policy (MSH-5) — containment and trust are one dial. Testimony buffer + death-watch are boot-graph nodes: hosted targets include them by default, freestanding freezes omit them. Fault handling is a capability package; nothing is imposed. Ratified 2026-07-04.

## ADR-017: Two-lane at-rest encoding; codecs are generated

Structured metadata (graphs, defaults, provenance, edit ops, kind nodes, ref values) encodes as IPLD **dag-cbor** (sorted keys, definite lengths, canonical floats, no NaN/Inf, links as first-class CID values); hashes are computed over these canonical bytes. Bulk payloads (audio, video, meshes, textures) are **raw-lane** bytes in kind-specific encodings, Merkle-chunked, hashed as-is (NaN legal — it's signal, not structure). **JSON is a projection**, tolerant by design: every human/agent surface speaks it; commit converts through the canonical encoder, so formatting can never fork identity. In-motion representation stays C/C++ structs; the translation is **generated from the single declaration** (the endpoints/schema struct, via the same reflection walk as AUT-3): in-motion layout, at-rest codec, and JSON projection from one source — the generated encoder is the ONLY writer of at-rest bytes, and round-trip property tests are auto-generated per kind. Hash-and-encode happens only at commit boundaries (commit, advertise, memo-lookup), never on tick paths. Declaration changes change hashes = kind evolution (its own design item). Node-type references in graphs pin CIDs; whether via a per-graph vocabulary-lock object is decided with the perpetual-greenfield item. **Projections are open-ended**: the three above are merely the privileged minimum — every kind may carry arbitrarily many further projections (HTML page, VR view, Markdown, TUI, …) as ordinary decoder programs (datasets: shippable, sandbox-run, declinable). Exactly ONE projection is authoritative for identity (the canonical at-rest bytes); a projection that publishes an inverse map is an editing surface (CMP-4 generalized) — this is the hypermedium/graphiverse mechanism. Ratified 2026-07-04.

## ADR-018: History is the op log

All mutation flows through structured edit ops (LNG-5); the session's history IS the op log — no snapshot semantics anywhere. Ops carry their inverses (set_param records the prior value; remove_node records the removed record and edges): undo = apply inverse, redo = re-apply. GESTURES ARE TRANSACTIONS: gesture brackets coalesce ops on the same route, so a slider drag is one undoable unit — retires the structural-vs-param snapshot heuristic and makes param gestures undoable. Snapshots demote to semantically-inert checkpoints (fast time-travel over long logs). Commit folds the log into the new object; the ref hash-trail is the coarse history, the op log the fine one, and a session's log is itself committable (provenance for "how did this patch get this way"). Ops are attributable (author peer key) — groundwork for audit and multi-writer collaboration. THE LOG IS A TREE; LINEARITY IS A VIEW: append-only, undo moves a cursor, an edit after undo branches instead of discarding the redo tail; the default UI is linear undo/redo along the current branch; the tree view is just another projection (ADR-017). Refs already fork at coarse granularity — one history shape at two zoom levels. Cherry-pick = deterministic replay of an op range; MERGE semantics deferred to the multi-writer design (same problem, one answer). Ratified 2026-07-04.

## ADR-019: One core, many hosts — machinery language and Python/JS interop

Core machinery is C++ (the kernel contract and freezer emit/consume it; NDK + Emscripten first-class; PFR→static-reflection generation path). Because machinery is invisible behind links, per-machinery substitution (e.g. Rust for a network-facing piece) stays legal forever without ontology impact; safety now comes from generated-smallness + fuzzing + quarantine containment (ADR-016/017). **No protocol logic is ever written twice**: other languages either LINK the one core — `import sygaldreye` is a trampoline; a CPython extension boots the same peer in-process, as the Emscripten build already does for the browser — or speak the GENERATED wire codecs when genuinely remote. Bindings are generated projections of the single declarations (Python dataclasses, TS types) — bindings that cannot rot. Graph-executes-Python and Python-executes-graph are the same embedding: Python/JS author worker-region node types (behavior links to source datasets; in-process, the hosting interpreter IS the executor's machinery), and graphs are Python callables (derivation mode: inputs → run-to-completion → outputs, provenance recorded), with edit ops as the API and span/audio payloads crossing zero-copy via the buffer protocol. Real-time regions stay native; a Python "oscillator" is a prototyping skin re-kerneled or frozen on promotion. Ratified 2026-07-04.

## ADR-020: Three disciplines forever; clocks are open

The rate lattice is exactly three DISCIPLINES — event (push), value (dirty-push/demand-pull, cadence-free), stream (clocked, parametric over WHICH clock) — and they are semantic categories, not frequencies. Cadence is not part of the type system: a clock is an executor's published output (ADR-015), so new "rates" arrive as new executors in capability packages (video 24fps, IMU 100Hz, MIDI pulse — which is events, not a sample clock) with no core change and no global rate enum. Kinds may pin clocks (audio pins block) as kind-node constraints (LNG-1). Regions are connected components under one clock's demand; crossings between clocked domains are boundary mappings (resamplers, jitter) supplied through the owning package's choose-adapters fan-in — visible and replaceable, as always. Ratified 2026-07-04.

## ADR-021: Determinism classes

Every derivation declares one of four classes — one more member of the promise family. **exact**: bit-reproducible everywhere (structural/integer work; graph compilation; codegen text); memo shared mesh-wide; verification = re-derive and compare hashes. **platform-exact**: reproducible given a platform fingerprint (arch + toolchain-closure), which joins the memo key. **approximate**: same semantics, different bits (float pipelines, GPU, threaded reductions); memo is first-write-wins per key (trust the deriving peer's signature — MSH-5 posture); provenance may hold a SET of outputs for one recipe; equality between them is kind-specific tolerance, never hash equality. **nondeterministic**: undeclared randomness / wall-clock / mid-derivation network — not memoizable, and honestly not a derivation: DECLARE THE INPUT OR IT'S A CAPTURE (seeded RNG is fine — the seed is an input); hidden nondeterminism is an honesty bug, lintable where detectable. Verification depth and eviction safety follow the class; derivation mode may opt into stricter float discipline (no fast-math, fixed thread order) to climb classes when reproducibility outranks speed. Ratified 2026-07-04.

## ADR-022: QoS-aware placement; predicted/posed edges

Requirements from types, offers from measurement, arithmetic at compile time, latency visible on the wire, prediction as a kind, policy as a pass. The mesh measures itself demand-driven: link RTT/jitter/bandwidth and peer load are net-package executor outputs (unmeasured unless demanded). An edge's discipline + clock IS its requirement (block crossings carry the block deadline; events need ordering; values tolerate staleness by pull construction); placement legality is first-order compile-time budget arithmetic — can some adapter satisfy this crossing on the measured link? Latency is REIFIED on net mappings (measured + declared, visible parameters); unsatisfiable placement fails loudly (fallthrough or unplaceable), never silent degradation. The placement policy (local > lowest-latency > least-loaded) is itself a replaceable pass. **Posed/predicted edges**: a kind whose value is a FUNCTION OF TIME (sample + timestamp + motion model), evaluated at the consumer's clock — extrapolation, not buffering (head pose → remote HRTF per block is the canonical case); prediction error is approximate-class semantics (ADR-021). Ratified 2026-07-04.

## ADR-023: Multi-writer — one live arbiter; history merges, not state

LIVE: every running instance has exactly one op queue (on its host peer); all writers — humans, agents, remote peers — submit attributed ops; ARRIVAL ORDER IS THE ORDER (game-server shape; the existing edit queue, formalized). Same-param collisions resolve by queue order (two hands, one knob); structural collisions are prevented by OP PRECONDITIONS (an op names what must exist; the loser fails loudly — LNG-5 addition). Presence is ordinary shared state. No optimistic-rollback machinery until lag is actually felt. APART: peers fork refs (provenance-or-fork) and grow op-tree branches (ADR-018); MERGE IS A DERIVATION — deterministic replay of an op range; precondition failures become a CONFLICT REPORT DATASET, resolved by authored resolution ops (agents welcome), the whole merge provenance-recorded. CRDTs are deliberately excluded from the core (live instruments need authority-now, not convergence-eventually; silent convergence = silent semantic surprise) but admissible per-kind behind links (e.g. collaborative text). The arbiter is supervised like everything else; the op log reconstructs the queue. Ratified 2026-07-04.

## ADR-024: Queries are graphs; the query vocabulary is core

A query is a graph wired from a small CORE vocabulary — traverse, filter, join, fixpoint (transitive closure; terminates by construction over finite stores) — part of the finely curated fundamental node set, because palette, search, lineage, back-links, and the store browser all stand on it. Run once = a derivation: the answer is an INDEX dataset with recipe provenance, memoized. Left running = a standing query: a reactive value whose cone commits dirty-push into (ADR-015) — incremental view maintenance for free. Indexes are datasets, hence providable; mesh queries fetch or fan out to providers' indexes; results are honest (bounded by reachable trust domain). STO-9's back-link index becomes an instance of this design, not a special case. A textual query syntax, if ever wanted, is a projection decoding to a query graph (ADR-017) — the core never grows a parser. Ratified 2026-07-04.
