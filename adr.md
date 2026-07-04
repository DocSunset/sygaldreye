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
