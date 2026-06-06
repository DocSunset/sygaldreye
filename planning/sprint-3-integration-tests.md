# Sprint 3: Linux Integration Tests

## Goal

Fill the testing gap between unit tests (individual components with stubs) and on-device
validation (Quest 3 headset required). Every critical signal path in the system should be
verified to work correctly on the Linux development machine.

## Baseline (before this sprint)

Running `build/host/components/*/component_name_test` produces:

| Component | Tests | Status |
|-----------|-------|--------|
| component_registry | 4 | PASS |
| eyeballs_node_abi | 14 | PASS |
| host_gl_context | 2 | PASS |
| noise | 5 | PASS |
| param_registry | 6 | PASS |
| particle_system | 3 | PASS |
| ray_selector | 3 | PASS |
| scene_graph | 3 | PASS |
| scene_snapshot | 1 | PASS |
| signal_graph | 6 | PASS (3 stale → rebuilt) |
| sphere_geometry | 4 | PASS |
| sygaldry_endpoints | 11 | PASS |
| terrain_generator | 3 | PASS |
| tree_generator | 3 | PASS |
| vr_panel | 4 | PASS (HitEdge bug fixed) |

**No tests at all for:** http_server, sky_dome, water_surface, aurora, lissajous,
chladni, reaction_diffusion, gl_program, rgba_shader, and any cross-component
integration of real visual nodes in the signal graph.

## What's missing and why it matters

All existing tests use stub nodes (ProducerNode/ConsumerNode hand-written in the
test file). No test has ever:

1. Registered a real node type (WaterSurface, SkyDome, Lissajous…) via
   `make_descriptor<T>()` and run it through `parse_graph` → `tick_graph`.
2. Called a visual node's `DrawFn` and verified no GL errors occur.
3. Made an actual HTTP request to the http_server and read a response.
4. Verified the signal propagation edge sky_dome.sun_elevation_out → water_surface
   actually moves the correct value.

These gaps mean the sprint-2 work (ABI v4, draw_calls, signal propagation) has never
been end-to-end verified on any platform.

## Fixes already made (before sprint 3 begins)

- **vr_panel.HitEdge** — one-line fix: `std::abs(normal.z())` → `std::abs(normal.y())`
  in the up_ref selection. Panels with Z-facing normal now map X → U correctly.
- **signal_graph_test binary** — rebuilt; all 6 tests now pass (3 were stale).

## Work items

Three independent tracks. Tracks 1 and 2 can proceed in parallel.

---

### Track 1: `integration_real_nodes` component

**What:** New component `components/integration_real_nodes/` with a test that
exercises the full ABI v4 lifecycle with real node types and verifies the signal
graph's propagation path end-to-end.

**Why:** Proves that sprint-2 ABI v4 additions (push_outputs, set_scalar_in,
push_draw_calls, port_schema) work with actual production node types, not stubs.

**Needs:** host GL context (visual node constructors and draw() need a current GL
context). Create `HostGlContext::create()` in a GTest `SetUpTestSuite` or at the top
of each test.

**Files:**
- `components/integration_real_nodes/CMakeLists.txt`
- `components/integration_real_nodes/integration_real_nodes.test.cpp`

**CMake:** Linux-only (`if(NOT CMAKE_CROSSCOMPILING)`). Uses NDK gtest
(`if(EXISTS ${GTEST_ROOT})`). Links against: host_gl_context, signal_graph,
component_registry, eyeballs_node_abi, water_surface, sky_dome, lissajous, chladni,
reaction_diffusion, terrain_generator.

Add `add_subdirectory(components/integration_real_nodes)` to top-level CMakeLists
in the unconditional section (the `if(NOT CMAKE_CROSSCOMPILING)` guard goes inside
the component's own CMakeLists).

**Tests to write (10 tests):**

```
RealNodes.GlContextRequired
  HostGlContext::create() → has_value() (guards rest of suite)

RealNodes.WaterSurfaceDescriptorLifecycle
  make_descriptor<WaterSurface>() → create() → process(0.0) → destroy()
  No crash; serialize() contains "cell_size"

RealNodes.WaterSurfaceSetScalarIn
  create() → set_scalar_in("cell_size", 2.5) → serialize() contains "2.5"

RealNodes.SkyDomePortSchema
  make_descriptor<SkyDome>()->port_schema non-null
  parse_port_schema(port_schema) → outputs contain "sun_elevation_out" (kind=scalar)
  and "render" (kind=draw_call)

RealNodes.SkyDomePushOutputs
  create SkyDome → process(0.0) → push_outputs with emit_scalar spy
  Verify emit_scalar is called with port_name == "sun_elevation_out"

RealNodes.DefaultGraphParsesAndTicks
  Register sky_dome and water_surface descriptors
  parse_graph with JSON:
    {"nodes":[{"id":"sky","type":"sky_dome","params":{}},
              {"id":"water","type":"water_surface","params":{}}],"edges":[]}
  → parse succeeds, 2 nodes
  → tick_graph(0.0) → draw_calls.size() == 2

RealNodes.EdgePropagationSkyToWater
  Same graph but with edge: {"from":"sky.sun_elevation_out","to":"water.sun_elevation"}
  After tick_graph: g.values["sky.sun_elevation_out"] holds a double (SkyDome default ~0.5)
  water_surface node's inputs.sun_elevation.value ≈ that value
  (Verify via set_scalar_in inspection: mock set_scalar_in on a wrapper, or read the
  WaterSurface pointer directly after tick)

RealNodes.AllVisualNodesDrawCalls
  Register sky_dome, water_surface, lissajous, chladni, reaction_diffusion, terrain_renderer
  For each: parse single-node graph → tick_graph → draw_calls.size() == 1

RealNodes.DrawCallNoGlError
  For WaterSurface: after tick, invoke draw_calls[0](Eigen::Matrix4f::Identity())
  glGetError() == GL_NO_ERROR

RealNodes.GraphSerializeRoundTrip
  parse_graph (sky + water + edge) → serialize_graph → parse again
  Second parse: 2 nodes, 1 edge with correct from/to fields
```

**Key implementation notes:**
- `HostGlContext::create()` must be called before any visual node constructor.
  Use a test fixture with `static void SetUpTestSuite()` that calls create() and
  stores the result in a static member.
- Visual node default constructors (WaterSurface, SkyDome etc.) may call GL functions.
  If they do, they need the GL context current before `d->create()`.
- The WaterSurface `set_scalar_in` path: ABI calls `set_scalar_in(node, "sun_elevation",
  v)` which maps to water_surface.inputs.sun_elevation.value. After a tick with the edge,
  read the WaterSurface pointer out of the graph: `static_cast<WaterSurface*>(g.nodes[idx].data)`.
- `terrain_generator` (TerrainRenderer) is in the unconditional build; use it if it has
  a `struct outputs { port<"render", DrawFn> render; }`. Otherwise use lissajous and chladni.

---

### Track 2: `integration_http` component

**What:** New component `components/integration_http/` that starts the HttpServer in a
background thread and makes real HTTP requests via raw POSIX sockets. Tests the REST
API surface that the companion app and external tools use.

**Why:** No HTTP test currently exists anywhere. The http_server.cpp has never been
exercised by any automated test.

**Files:**
- `components/integration_http/CMakeLists.txt`
- `components/integration_http/integration_http.test.cpp`

**CMake:** Unconditional (no GL or Android-specific deps). Uses NDK gtest. Links:
http_server, signal_graph, component_registry, eyeballs_node_abi.

**HTTP client helper:** Write a minimal inline helper (≤40 lines) in the test file:
```cpp
// Returns {status_code, body} or {0, ""} on error.
std::pair<int,std::string> http_get(int port, const std::string& path);
std::pair<int,std::string> http_post(int port, const std::string& path,
                                     const std::string& body);
```
Uses POSIX: `socket/connect/send/recv` with a blocking 500ms timeout (SO_RCVTIMEO).
For SSE: open a socket, send GET /events, read lines until `data:` prefix seen or
timeout.

**Tests to write (8 tests):**

```
Http.GetParamsReturns200
  Register a GET /params handler returning "{\"ok\":true}"
  Start server on a free port (bind to :0, getsockname to find port)
  http_get(port, "/params") → status 200, body contains "ok"

Http.PostParamsDeliverBody
  Register POST /params handler that echoes body as JSON value
  http_post(port, "/params", "{\"val\":42}") → body echoed back

Http.UnknownPathReturns404
  http_get(port, "/nonexistent") → status 404

Http.GetPaletteListsTypes
  Register a node type (minimal stub descriptor)
  Register GET /palette handler that returns serialize_graph({}) or type_names
  http_get(port, "/palette") → body contains the type name

Http.PostGraphAcceptsValidJson
  Register POST /graph handler that parses and returns "{\"nodes_parsed\":N}"
  Post a 2-node graph JSON → 200, nodes_parsed = 2

Http.BroadcastEventReceivedBySseClient
  Start server; connect to /events (leave socket open)
  In another thread: server.broadcast_event("graph", "{\"test\":1}")
  Read from SSE socket with 1s timeout → line starting with "data:"

Http.MultipleSequentialRequests
  Send 5 GET /params requests back-to-back → all 200 OK

Http.ConcurrentRequests
  Send 3 requests from separate threads concurrently → all 200 OK
```

**Key implementation notes:**
- Use port 0 (OS assigns) to avoid port conflicts between test runs.
- The SSE test needs a 1–2 second timeout; use `select()` or `SO_RCVTIMEO`.
- The HttpServer::start() method takes get_handler and post_handler params for /params.
  Use `add_route("GET", "/palette", ...)` for the palette test.
- Stub descriptor for the palette test: fill only `type_name` and `version`; set all
  function pointers to nullptr; register with `reg.register_builtin(&desc)`.

---

### Track 3: Shader compilation tests (lower priority, do after Tracks 1 and 2)

**What:** Add test targets for `gl_program` and one representative shader
(`rgba_shader`) that verify: GL context + compile a real shader → no GL errors.

**Files:**
- `components/gl_program/gl_program.test.cpp` (new)
- `components/rgba_shader/rgba_shader.test.cpp` (new)

**Tests:**
```
GlProgram.CompileMinimalVertFrag
  HostGlContext::create()
  GlProgram::create(minimal_vert_src, minimal_frag_src) → succeeds
  glGetError() == GL_NO_ERROR

RgbaShader.InitAndDraw
  HostGlContext::create()
  RgbaShader s; s.init()
  Invoke with a trivial VAO → glGetError() == GL_NO_ERROR
```

---

## Building and running

All integration tests rebuild with:
```bash
nix develop --command cmake -S . -B build/host
nix develop --command cmake --build build/host --target integration_real_nodes_test
nix develop --command cmake --build build/host --target integration_http_test
./build/host/components/integration_real_nodes/integration_real_nodes_test
./build/host/components/integration_http/integration_http_test
```

Add `sh/test_host.sh` to run all host test binaries in sequence and report failures.

## Acceptance criteria

All host test binaries pass with 0 failures. Specifically:
- `integration_real_nodes_test`: 10 tests, all PASS
- `integration_http_test`: 8 tests, all PASS
- Existing tests remain passing (no regressions)
- `vr_panel_test`: 4 tests, all PASS (already fixed)
- `signal_graph_test`: 6 tests, all PASS (already fixed)

## Dependencies between tracks

Tracks 1 and 2 are fully independent; run in parallel.
Track 3 depends only on host_gl_context being proven to work (Track 1 does this
implicitly), so start it after Track 1 is underway but don't wait for it to finish.

## Out of scope for this sprint

- vr_editor integration tests (depends on OpenXR headers; can't run on Linux without
  header stubs)
- renderer_node integration tests (Android-only CMake section)
- Audio synth integration tests (AAudio; Android-only)
- On-device tests (headset detached this sprint)
