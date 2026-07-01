# Ticket Sequencing Rationale

## Major Phases

### 000–004: CMake Visibility
Fix `PRIVATE`/`PUBLIC` linkage in every component's `CMakeLists.txt` first. These changes are purely build-system and have no code conflicts, but every subsequent header-isolation change assumes they are already in place (otherwise a component that "should not" see a transitive header still gets it through PUBLIC linkage).

Order within phase: lowest-level component first (`vr_math` → `gl_program` → `cube_mesh` → `input` → `renderer`).

### 005–017: RAII and Resource Safety
Add destructors, deleted copy ops, and move semantics before any other header work. Rationale: later tickets that move members to private or restructure the API assume the types already own their resources correctly. Doing RAII first avoids rewriting destructor logic twice.

Order within phase: component dependency order — `gl_program` (no deps) → `cube_mesh` (uses `gl_program`) → `xr_session` → `input` → `renderer` (highest-level, depends on all others).

For each component the order is: RAII header (`api_raii_*`) → RAII implementation detail (`cpp_raii_*`) → "no invalid default" factory pattern (which builds on the destructor) → temporal precondition assertion (builds on knowing the object is now valid-or-assert).

### 018–026: Static State → Instance Members
Three triplets (one per component: `xr_session`, `renderer`, `input`), each ordered: API-level description → C++ mechanics ticket → testability ticket. The testability tickets (`test_no_static_state_*`) validate that the change was made and that per-object semantics are restored; they must follow the implementation tickets.

### 027–030: Encapsulation / Private Members
Make data members private on `XrSessionObj` and `Renderer`, and express lifetime dependencies in the API. These come after RAII (which introduced the members) and after static-avoidance (which added new members that also need to be private).

### 031–034: Header Isolation
Remove platform-specific and implementation-only includes from public headers. These come after encapsulation (so we know which members are private and can be forward-declared) and after CMake visibility (so removing an include doesn't accidentally break a consumer that was relying on leakage).

Order: `xr_session` → `input` → `renderer` → `cube_mesh`. `renderer.hpp` includes both `scene.hpp` and `cube_mesh.hpp`; the renderer header-isolation ticket is a prerequisite for the dependency-inversion work in Phase 10.

### 035–044: Type Safety and Error Propagation
Replace raw `int` hand indices with `enum class Hand`, replace `bool valid` flags with `std::optional`, and make `sync()`/`poll_events()` return errors instead of `void`. These come after header isolation (the new types appear in headers that are now clean) and after RAII (objects are safe to construct and pass by value).

Order within phase: input type safety (typed index → optional return) → scene type safety (optional parameters) → error propagation (xr_session first, then input, then consistency/test).

### 045–051: Named Constants and Minor C++ Quality
Mechanical, low-risk changes (constexpr, named constants, safe strncpy). Placed after structural changes so they don't conflict with lines being moved or deleted.

### 052–056: Span/Lifetime Contracts and API Documentation
Document span invalidation and add `///` header comments. These are documentation-only or trivially additive; placed here so they capture the final API shape established by the previous phases.

### 057–064: Architectural Restructuring
Large behavioral refactors: fix the fragile `cubes_.resize` pattern in scene, move hand-transform logic out of `app.cpp` into `scene`, invert the renderer→scene dependency, split single-responsibility violations, and enforce explicit initialization ordering in `Renderer`.

These come late because:
- They depend on `std::optional` parameters being in place (039 before 058).
- `arch_dep_inversion_renderer_hpp` (061) requires header isolation (033) to be done first — otherwise `renderer.hpp` still includes `scene.hpp` and the inversion can't be expressed.
- `arch_single_responsibility_renderer_cpp` (062) and `arch_single_responsibility_app_cpp` (063) both presuppose the dependency inversion is resolved.
- `arch_explicit_ordering_renderer_cpp` (064) restructures the init sequence, which is the highest-risk change in this group and goes last.

### 065–075: Performance
All perf tickets come after correctness and architecture are stabilized:
- `perf_fixed_scene_structure_scene_hpp` (065) must precede `perf_no_alloc_hot_path_scene_cpp` (066) because the hot-path ticket assumes the fixed-array structure.
- GPU state minimization tickets (073–074) precede batch draw calls (075) because batching builds on the bind-once pattern.
- `perf_no_alloc_hot_path_xr_session_cpp` (067) comes before `perf_frame_budget_observability` (071) since removing the per-frame vector allocation simplifies the instrumentation.

### 076–084: Testability
Tests and test-enabling extractions come last. They depend on all structural and architectural changes having stabilized the interfaces:
- `test_edge_case_coverage_vr_math_cpp` (076): pure math, can be done anytime but fits here as a baseline.
- Pure function extraction tickets (077–079): require the functions to already be structurally separated (arch_single_responsibility done in 062–063).
- GL error detection (080–081): require the GL resource lifecycle to be correct (RAII done in 005–017).
- State machine test (082): requires error propagation from `poll_events` (040) and private members (027) to be in place.
- Span invalidation and ordering invariant tests (083–084): require the fixed-array scene structure (065) and the separated world/hand cube fields (057–058) to be done first.

## Key Non-Obvious Ordering Decisions

- **RAII before encapsulation**: `api_encapsulation_xr_session_hpp` (027) makes all data members private. It must come after the RAII tickets (012–013) because those add destructor-related members that need to be declared private too. Doing encapsulation first would require reopening the same header.

- **Static avoidance before testability**: The three `test_no_static_state_*` tickets explicitly state that static locals "make test isolation impossible." They are placed immediately after the corresponding implementation tickets (018–026) rather than in the test phase, because they validate a safety property rather than add new test coverage.

- **Header isolation before dependency inversion**: `arch_dep_inversion_renderer_hpp` (061) cannot be done while `renderer.hpp` still includes `scene.hpp`. The header isolation ticket (033) removes that include as part of cleaning up renderer.hpp. These two tickets overlap in the same file and must be serialized: 033 before 061.

- **Scene cohesion before layer ownership**: `cpp_design_cohesion_scene_cpp` (057) and `arch_layer_ownership_scene_cpp` (058) both modify `scene.cpp` and overlap with the `set_controller_poses` signature. The cohesion ticket separates world and hand cube storage; the layer-ownership ticket moves hand-transform computation into scene. Both must precede `arch_layer_ownership_app_cpp` (060), which removes the computation from `app.cpp`.

- **arch_value_semantics_scene_cpp (039) before arch_layer_ownership_scene_cpp (058)**: The layer-ownership ticket passes raw `XrPosef` to scene; the value-semantics ticket changes `set_controller_poses` to accept `std::optional<Eigen::Matrix4f>`. Doing value semantics first gives scene a cleaner receiving API before the computation moves in.
