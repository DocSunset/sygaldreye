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

## ADR-006: Nodes are C-ABI descriptors with pfr-derived endpoints (ABI v6)

Every node is one `endpoints` struct (in / normalled_in / cv_in / out / event_*); `make_descriptor` derives the port schema via pfr and exports a C-ABI `EyeballsNodeDescriptor`. Edges are literal pointers (connect/output_ptr). One declaration serves execution, serialization, schema, and plugin loading.

## ADR-007: Conformability lifting is one executor rule (ABI v7)

"Match cells, lift frames, broadcast scalars." A port's cell rank derives from its kind; the executor lifts on excess rank only — one detection in `build_plan`, one strategy table (Clones / CellMap / Scan). Resource-holders are unliftable: plan-time error. ABI v7 adds `lift_kind`/`lift_key`. No node hand-rolls a lift.

## ADR-008: render_region is the sole GL boundary (ABI v8)

Nodes emit declarative `Mesh` + `Surface` payloads; `render_region` owns all programs, geometry, and draw calls. `DrawFn` (callbacks over edges) is retired — typedef, ABI fn pointers, and both shells' draw loops deleted. Draw order is the `seq` event chain, not callback order.

## ADR-009: The editor is graph content, not shell chrome

Cards, wires, gestures, and palette are nodes + subgraphs over the `graph_source`/`edit_sink` meta seam (graph as data in, structured edit ops out). The VrEditor/EditorNode monolith is deleted; a scene that wants editing carries the editor rig, and extending the editor is a graph edit.

## ADR-010: DSP is authored as per-sample kernels + compile-time block shells

Kernels (`synth_core/kernels.hpp`) are plain structs with `tick()`: dt-based, no absolute-time state, no allocation in tick. `Lift` (per-channel map × frame scan) and `Gen` stamp the block shell at compile time. The kernel is the lifting/freezing unit; shells own buffers and pacing.

## ADR-011: Generic editor-context descriptor hook (ABI v9, 2026-07-01)

`pump_contexts` had regressed into ~13 per-type `static_cast` special cases — plugins and subgraph-nested gesture nodes silently got no context. ABI v9 adds one generic editor-context hook on the descriptor; any node declaring it (`set_host_context(kind, ctx)`) receives the graph / edit-queue / overrides / registry context — including inside subgraphs, which forward the hook — and the per-type shell dispatch is deleted.
