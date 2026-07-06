# The Embodiment Plan — from headless-green to hands-in-the-graph

_Status 2026-07-06: 161 pass / 0 fail / 5 pending. Everything buildable
headless is green and audited. This plan covers the remainder: real pixels,
a host shell, the editor surface as graphs, the Quest target, the xr
package, and real audio devices — ending with the editor operable
in-headset (PKG-3.1), authoring graphs from scratch, editing itself, and
the view pose rewired into your own hand (PKG-3.3, new)._

**Read BUILDER.md first. It still binds — every rule, the trap list, the
Judgement section.** This plan does not replace the loop; it FEEDS it.
Phase 0 puts new criteria into the book and the manifest, after which
`python3 conformance/run.py` points at this work the same way it pointed
at rungs 1–12. When phase 0 lands, this document demotes itself to
chapter-guidance: the runner is the to-do list, this file is the "why and
exactly how" beside it.

## The governing principle (the right answer, fixed in advance)

C++ is permitted **only** where the platform forces a hand, and each such
file is one `// clause: machinery` component behind a package contract.
Everything a person sees, touches, or edits — cards, wires, labels,
palette, layout, gestures, the eyeball-in-hand rewiring — is **graphs**,
composed from a small enumerated leaf vocabulary.

The complete set of new natives this plan authorizes is in Appendix A.
**If you find yourself writing a native not on that list, stop.** Either
the thing is expressible as a graph over existing vocabulary (author it),
or a LEAF is missing (add the leaf to Appendix A's shape — a one-line
generated stamp or a kind constructor — record it in
`conformance/STRENGTHENINGS.md`, and then author the graph). A native
named after an editor concept (`card_*`, `palette_*`, `wire_drag`,
`gesture_*`) is a violation on sight — the probe built those in C++ and
that is precisely the compromise the greenfield exists to erase.

## Phase order and dependencies

```
0 (gates)
└─ A (pixels, host headless)
   └─ B (host shell: window + input sources + mesh drive)
      └─ C (editor surface as graphs)  ← host editor complete here
   └─ D (Quest toolchain + boot)       ← needs A only
      └─ E (xr package)                ← needs C for PKG-3.1
      └─ F (audio devices, FRZ-4.1)    ← device half needs D
```

Rough sizing (builder sessions): 0 ≈ ½, A ≈ 1–2, B ≈ 1, C ≈ 2–3,
D ≈ 1–2, E ≈ 2–3, F ≈ 1–2 + hardware-in-hand time. Rung 10 stays
skipped; nothing here depends on it.

---

## Phase 0 — Author the gate first (no implementation code in this phase)

The method is test-first at the criterion level. Before any GL call
exists, the book gains the criteria, the manifest regenerates, and the
tests land pending. Every addition below is a Judgement §1 strengthening:
record each in `conformance/STRENGTHENINGS.md` (id, what, why).

### 0.1 New criteria (exact placements)

Add to `architecture/07-capability-packages.md` under PKG-4:

- **PKG-4.3 (pixels are real):** `syg frame` renders a fixture graph
  headless (surfaceless EGL) to raw RGBA; property checks per
  `fixtures/golden-frame.md` pass: exact clear color, drawn-geometry
  coverage fraction within bounds, color centroid within tolerance; a
  scripted `set_param` op between frames moves the centroid in the
  predicted direction; a draw node OUTSIDE the head's chain contributes
  zero coverage. (The pkg42 chain semantics, now in pixels.)
- **PKG-4.4 (the shell is the same peer):** a windowed host shell is the
  ordinary peer with the render executor presenting to a window (or
  offscreen surface under test); pointer and key input reach the graph
  ONLY as package source nodes; a scripted offscreen session drives a
  pointer gesture that adds a node and the next frame's properties change
  accordingly.

Add to `architecture/07-capability-packages.md` under PKG-3 (gated on
ADR-037, below):

- **PKG-3.3 (the view is an edge):** with the editor live in-headset,
  rewiring the head's `view_pose` input from the head_pose source to a
  controller pose source is a single live graph edit — no restart, no
  recompile of natives; undo restores it. Scripted variant runs against
  simulated pose sources; the on-device run is blessed by Travis (he is
  the testimony — and the one who asked to hold his own eyeballs).

Add to `architecture/09-editor-documents.md`:

- **EDR-1.2 (the surface is graphs):** every editor-surface behavior —
  cards, wires, labels, palette, layout, gesture logic — lives in
  `graphs/editor/*.json` over the render/text leaf vocabulary; the set of
  registered natives exactly equals `vocabulary/packages.json` (SZ-2
  already checks this) and that file's diff for this work contains ONLY
  the Appendix-A names.
- **EDR-7.2 (authoring from scratch):** a scripted gesture session in the
  shell, starting from an EMPTY graph, authors hello-cosine end-to-end
  through pointer-source-driven gesture ops (spawn from palette, wire,
  set params, save); the saved doc's topology matches the fixture (modulo
  generated ids) and `syg render-graph` of it passes the golden-audio
  properties.

Then: `python3 conformance/extract_manifest.py` (never hand-edit the
manifest), add pending tests to `conformance/tests/` (see each phase's
witness section for exactly what they assert), and extend
`conformance/HARNESS.md` with the new subcommand rows in the same commit
(FMT-5 discipline).

### 0.2 ADR-037 draft (the view is an edge)

Write the draft in `adr.md` now, hot (Judgement §4); Travis ratifies
before Phase E builds it:

> **ADR-037: The view is an edge (DRAFT).** The render/xr head takes
> `view_pose` (position + orientation; catalog kinds vec3 + quat) as an
> ordinary INPUT port, default-wired from the `head_pose` source node.
> Machinery composes per-eye views by expressing the runtime's
> head-to-eye transforms (xrLocateViews) relative to the incoming pose.
> Rationale: N4 (humans and agents share paths) and L22 — where the
> camera sits is authoring, not machinery; "eyeballs in your hand" must
> be an edit, not a hack. Eye projection matrices stay machinery.

### 0.3 Tighten the existing GL gate

PKG-4.1's grep currently targets a `components/` path (probe-era; vacuous
against `src/`). Strengthen it:
`grep -rnE "gl[A-Z]|egl[A-Z]" src/ | grep -v "^src/render/" | grep -v "^src/xr/"`
must be empty. Record in STRENGTHENINGS.md. (After Phase A this bites for
real: the ONLY files naming GL are the two render-machinery components.)

### 0.4 New fixture spec

Write `fixtures/golden-frame.md`, modeled clause-for-clause on
`fixtures/golden-audio.md`: **property checks, never byte-golden images**
(GPU rasterization varies across Mesa/Adreno; byte-comparison would be a
lie that passes on one machine). Properties: background pixels equal the
clear color exactly; foreground coverage fraction within [lo, hi];
coverage centroid within a tolerance box; monotone predicted motion under
a param sweep. Include the blessing protocol: first light goes to Travis
as a PNG; he blesses; the take is committed as a capture.

---

## Phase A — Pixels (render machinery, host headless)

**Goal:** PKG-4.3 green. The render package gains real machinery behind
the contracts that are ALREADY green — do not touch `instanced_draw`
(its chain semantics are permanent and tested) and do not alter
`render_head`'s event-chain behavior (pkg42 pins it).

### A.1 New vocabulary (all entries → `vocabulary/packages.json` render/text)

| native | clause | what |
|---|---|---|
| `draw` | floor (passive boundary, like `dac`) | consumes `mesh` + `surface` values and the head-chain tick; holds the latest pair; emits chain onward. NO GL — the executor reads it. |
| `mesh_from_spans` | generated (kind constructor) — **core vocabulary**, beside `spanv` | positions span (+ dx/dy translate inlets) → one `mesh` value. Pure data assembly. |
| `surface_flat` | generated (kind constructor) — **core vocabulary** | r/g/b/a scalars → a `surface` value naming the built-in `flat` program. |

Packaging note (pkg41 enforces this): the render PACKAGE must contain
EXACTLY the chain-port types (`render_head`, `instanced_draw`, `draw`) —
the constructors go in core. The GL machinery component must be named
`render_region` (the book's name; the pkg41 gate exempts that filename).

The `mesh`/`surface`/`texture` kinds already exist in
`vocabulary/kinds.json` — extend their entries only if a decoder name is
missing; do not add kinds.

**Recorded gap (flag duty, note it in status.md when you commit):**
surfaces in v1 name one of two machinery-owned programs (`flat`,
`glyph`). Shaders-as-committed-datasets is a later succession of the
surface kind; v1 deliberately does not attempt it.

### A.2 New machinery (the only GL in the repo)

Follow the mesh package's layout convention exactly
(`src/mesh/identity/`, `src/mesh/link/` — component dir, hpp/cpp,
CMakeLists, clause on line 1):

- `src/render/egl_surface/` — `// clause: machinery — EGL context
  creation: surfaceless (headless), window (host), and Android
  (Quest, Phase E)`. Salvage source: `probe/components/egl_context/`
  (85+29 lines) and `probe/components/host_gl_context/`. GLES 3.2,
  matching the Quest.
- `src/render/render_region/` — `// clause: machinery — the GL boundary:
  the render package's executor. The ONLY file that issues draws.`
  Owns: the two built-in programs (salvage shader source from
  `probe/components/render_region/render_region.cpp`, 266 lines, and
  `probe/components/gl_program/`, `gl_geometry/`), geometry upload with
  persistent-slot reuse (the probe's `update_verts` pattern), texture
  upload (stb_image as maturity import for the atlas PNG, Phase C),
  frame begin/present, and RGBA readback for `syg frame`.

The executor's contract, prescribed: at each frame boundary it walks the
realized plan's `draw` instances IN HEAD-CHAIN ORDER (the order pkg42
already proves), uploads any changed mesh/surface values, issues the GL
calls, presents (or reads back). Nodes stay passive; the region owns the
device — the same shape ADR-015 gives audio, and the pattern-setter for
every device executor after it (xr, audio out).

### A.3 The witness

`syg frame` (add the HARNESS.md row in the same commit as the test):

```
| A | syg frame | {"graph":<interchange>,"frames":N,"size":[w,h],
    "ops":[{"frame":k,"route":...,"value":...}]} on stdin →
    N raw RGBA8 frames concatenated on stdout;
    {"frames":N,"ns_per_frame":x} on stderr |
```

Test (`conformance/tests/` — extend rung08.py or add a phase file
following its conventions): fixture graph = `render_head` → `draw` fed by
`mesh_from_spans` (one triangle from a spanv) + `surface_flat`; plus one
STRAY draw not in the chain. Assert every golden-frame property including
the stray contributing nothing. Helper: write
`_helpers.golden_frame_check` beside `golden_audio_check`, same spirit.

### A.4 Traps (each of these bit the probe; the scars are in status.md)

- Resource acquisition in `create()` — L9. GL objects are acquired on
  the executor's first frame, loudly, never in node create.
- Headless renders into an FBO: **never bind FBO 0**; save/restore
  framebuffer + viewport; the region owns its VAO.
- `#version` must be the first line of every shader — Adreno tolerates
  violations, Mesa refuses (found 2026-06-10).
- Float textures need NEAREST in core GLES3.
- Do not resurrect `DrawFn`, `Scene`, or any probe render node wholesale
  — `render_payloads.hpp` and `render_region.cpp` are REFERENCE READING
  for shapes; the greenfield draw is values through the existing kinds.
- Nix loader vs system GL on host: run outside `nix develop` or with
  `LD_LIBRARY_PATH=/usr/lib` if EGL init fails mysteriously (probe
  lesson, 2026-06-10).

**Blessing point:** first triangle → PNG → Travis. Documentary moment
(first light — log it).

---

## Phase B — The host shell (a peer you can see and poke)

**Goal:** PKG-4.4 green. A long-running host process that is the ORDINARY
peer — booted from a tape, organs live, engine slot — with the render
executor presenting to a GLFW window, and input entering as source nodes.

### B.1 New vocabulary + machinery

| item | clause | what |
|---|---|---|
| `pointer` (native, core vocab) | generated shell | source node: vec2 position + button/trigger scalars. Executor-fed. |
| `key_text` (native, core vocab) | generated shell | source node: text events. Executor-fed. |
| `src/host/window/` | machinery | GLFW window + event pump feeding pointer/key_text instances; GLES 3.2 EGL context via `egl_surface` (match the Quest — probe did exactly this). |
| `src/trampoline/shell_main.cpp` | machinery (trampoline, ADR-019) | boots the peer from the embedded tape + a named graph; runs frame loop; joins the mesh on loopback. |

`syg shell {graph, offscreen?, port?}` — runs until quit. Under test,
`offscreen` uses the surfaceless context and the readback path.

### B.2 The agent path is the mesh — no side doors

The probe had an HTTP surface. The greenfield **must not**: MSH-2 says no
pre-auth service surface, and rung 9 already built the real thing. The
shell advertises on loopback with the ADR-035 handshake; agents drive it
with ops-to-arbiter over the wire. Add to `syg mesh` a `connect
{host,port}` op so a session can address an OUT-OF-PROCESS peer (today it
simulates N in-process peers; this strengthens the CNF-2 shape into
inter-process reality — record the strengthening).

Pairing for the local loop: the shell reads an accepted-keys file beside
its objdir; `syg mesh` clients pair with a key minted per session. Wire
format, refusals, framing: all already pinned (ch. 14, ADR-035) — reuse
`src/mesh/link`, write nothing new on the wire.

### B.3 Witness

Scripted offscreen test: boot shell with a two-node graph → connect via
mesh → send a gesture op (add_node through the arbiter, exactly what a
pointer-driven gesture graph will emit in Phase C) → `frame` readback
changes per golden-frame properties. Human parity comes in Phase C
(EDR-7.2 runs the same script through real pointer sources).

**Trap:** do not let the window pump call into the plan directly. Events
land in source-node state via the executor's boundary drain — the same
arbiter/tick-boundary discipline as every other input in the system.

---

## Phase C — The editor surface as graphs (the L22 phase)

**Goal:** EDR-1.2 and EDR-7.2 green. This is the phase where the
temptation to write C++ is strongest and the authorization is narrowest.
The probe's editor-surface C++ (`card_widgets_mesh`, `card_labels_mesh`,
`editor_wires`, `palette_mesh`, `editor_layout`, `handle_picker`,
`wire_drag`, `slider_drag`, `dwell_delete`, `palette`) is **reference
reading for BEHAVIOR only** — what a card shows, how a drag feels, what
the layout constants were (0.018 m row pitch, eye-level 4-wide grid).
Not one of them returns as a native.

### C.1 The two text leaves (the only new natives in this phase)

| native | clause | what |
|---|---|---|
| `glyph_quads` | floor (pure layout math) | text in → quad-position/uv spans out, using atlas METRICS read from a committed dataset (fetched by hash — not a baked header). Salvage the layout algorithm from `probe/components/text_mesh/glyph_layout.hpp`. |
| *(atlas upload)* | — | not a node: `gl_region` uploads the atlas texture (dataset bytes → stb_image decode → texture) and owns the `glyph` MSDF program (salvage shader from `probe/components/text_label/`). |

The atlas itself: salvage `probe/assets/fonts/dejavu_sans.{png,json}`
(regeneration recipe: `probe/sh/gen_font_atlas.sh`, msdf-atlas-gen,
chars 32–126) and commit both as datasets via `chunk-put`; the boot tape
or the editor graph references them by CID. **The font travels the same
way every other byte in this system travels.**

### C.2 The graphs (author in this order; each is a commit)

All under `graphs/editor/`:

1. `card.json` — one card: `mesh_from_spans` quad at a position inlet +
   `glyph_quads` label + per-port handle quads. Inlets: id, position,
   port count. (The probe's `card.json` is the shape ancestor; the
   greenfield `graphs/card.json` walk-fixture stays untouched.)
2. `cards.json` — `graph_source` (core vocab, exists) → lift-expanded
   `card` keyed by node id (the rung-5 lift machinery, keyed subgraphs —
   this exact composition was proven in the probe on 2026-06-15).
   Grid layout = arithmetic leaves over the index span (mul/add — they
   exist).
3. `wires.json` — edge list from `graph_source` → line-strip meshes.
4. `palette.json` — registry-face names (exists) → rows of label +
   spawn zone. (`palette_osc.json`/`palette_noise.json` already exist as
   EDR-1.1 swap fixtures — extend, don't fork, the pattern.)
5. `gestures.json` — pointer → gesture FSMs → **edit ops into the
   arbiter**, via `op_button` (exists; it is exactly what `syg edit`'s
   agent-drive already bangs). Pick = nearest-handle arithmetic (lt/
   fmin over spans); drag FSM state lives in `cell`/`counter` nodes;
   dwell = counter + threshold. If an FSM genuinely cannot be expressed,
   the missing piece is a LEAF (e.g. a hysteresis one-liner via the
   generator stamp) — never a gesture native.
6. `editor.json` — the composition root wiring 1–5 into the draw chain,
   default graph for `syg shell`.

Behavioral constants from the probe worth honoring (they were tuned):
grip-gated delete (bare-hover delete was a bug — 1 s gaze deleted
nodes), nearest-handle wire drop (not first-in-radius), slider ranges
from kind metadata (`kinds.json` widget entries — already designed for
exactly this).

### C.3 Witnesses

- **EDR-1.2:** assert `syg palette` output == `vocabulary/packages.json`
  (exists via SZ-2) AND the editor-behavior grep: no `.cpp` under `src/`
  matches `card|palette|wire_drag|slider_drag|dwell|gesture` (add to
  `gates.sh` beside the other law greps).
- **EDR-7.2:** the from-scratch authoring script (Phase B's mesh drive,
  now through the full gesture graphs): empty graph → spawn osc, vca,
  dac from palette → wire → set freq/gain → save → topology matches
  hello-cosine fixture modulo ids → `syg render-graph` passes
  golden-audio properties.
- **Self-edit in pixels:** re-run EDR-1.1's palette swap (green in the
  harness since rung 11) inside the shell and screenshot-check the
  palette visually changed. The editor edits itself the moment it is
  visible, because it was already doing so headless — this witness just
  proves nothing about that broke on the way to the screen.

**Blessing point:** Travis authors something in the windowed editor with
his own hands. Documentary moment (the first graph authored by hand in
the greenfield).

---

## Phase D — The Quest target (toolchain + boot, no XR yet)

**Goal:** the greenfield core boots and passes its harness on-device.
XR comes after boot is boring.

### D.1 Toolchain (flake changes, pinned like the probe's)

Add to `flake.nix`: Android NDK **27.3.13750724**, platform 34 /
`android-26` min, `arm64-v8a`, `c++_static`, built via the NDK's own
`android.toolchain.cmake` (the probe passes it directly — copy
`probe/sh/build.sh`'s invocation); OpenXR-SDK **1.1.60** as a flake input
(loader for Phase E); keep host packages unchanged. The escapement is
already `-ffreestanding`-clean (COR-1) and FRZ-3.1 already cross-links
arm — the core will move quietly; the organs/executor are a plain NDK
port (POSIX sockets in `src/mesh/link` work as-is on Android).

### D.2 The device trampoline + rig

- `src/trampoline/quest_main.cpp` — android_native_app_glue entry,
  boots the peer exactly as `shell_main` does, EGL up FIRST and never
  dependent on XR (probe scar: launching with the headset un-worn left a
  zombie — `probe/components/app/app.cpp:316` documents it).
- `apps/quest/AndroidManifest.xml` — new package id (`com.sygaldreye.peer`;
  do not reuse `com.eyeballs.vr`), permissions INTERNET + WIFI_STATE +
  CHANGE_WIFI_MULTICAST_STATE (mesh + mdns; probe also grabs a multicast
  lock — salvage that call). RECORD_AUDIO only when mic work returns.
- `nix/`-style scripts mirrored from the probe: build / package (debug
  keystore) / run (`adb install -r`, signature-mismatch reinstall) /
  test (push test binaries to `/data/local/tmp`, run via adb) — salvage
  `probe/sh/{build,package,run,test}.sh` nearly verbatim; they are
  tooling, not core, and the salvage ban does not apply to them.

### D.3 Witness

On-device harness subset via adb: `render-movement` byte-check against
golden audio properties, `boot-audit` over an empty objdir, `queue-audit`
(threads are where cross-compiles lie). Then: shell boots on-device
rendering the editor graph FLAT to the swapchain-less EGL surface — the
proof that everything except XR is device-clean.

---

## Phase E — The xr package (PKG-3.1, 3.2, 3.3)

**Goal:** rung 8's remaining Quest criteria. Requires ADR-037 ratified
(0.2) and Phase C's editor graphs — because PKG-3.1's editor is the SAME
graphs with different sources plugged in. That's the payoff of the whole
source-node discipline: in-headset support means swapping which sources
feed `gestures.json`, not porting an editor.

### E.1 Vocabulary + machinery

| item | clause | what |
|---|---|---|
| `head_pose`, `controller` (natives, xr vocab) | generated shells | pose source nodes (vec3 + quat outs, trigger/grip scalars on controller). Executor-fed — `pump_xr_sources` never returns (PKG-3.2's grep). Reference: `probe/components/xr_sources/`. |
| `src/xr/session/` | machinery | OpenXR instance/session/state machine. Salvage: `probe/components/xr_session/` (111+58) + `probe/components/app/xr.cpp` (124). |
| `src/xr/frame_loop/` | machinery — **the xr executor** | xrWaitFrame/Begin/End owns the region's cadence (PKG-3.1: "the xr executor owning the frame loop"); locates views; feeds pose sources at the boundary drain; hands eye targets to `gl_region`. Salvage: `probe/components/frame_loop/` (63) + `renderer/` (142) + `eye_swapchain/` (158). |

Per ADR-037, `render_head` gains the `view_pose` input, default-wired
from `head_pose`. The frame loop expresses xrLocateViews' head-to-eye
transforms relative to the INCOMING pose — so when the pose comes from a
controller, both eyes come along for the ride.

### E.2 Witnesses

- **PKG-3.2:** `grep -rn pump_xr_sources src/` empty; a plan-level
  assert that pose data reaches the graph only through registered source
  instances (same shape as MSH-3.1's advertisement query witness).
- **PKG-3.1:** the editor session of EDR-7.2 re-run on-device with
  controller sources driving the same gesture graphs; plus Travis
  in-headset, blessing. (Agent script and human hands, same graphs —
  EDR-7's parity doing its job.)
- **PKG-3.3:** scripted against simulated pose sources: rewire
  `view_pose` from head_pose to controller/pose mid-session via one
  `add_edge`/`remove_edge` gesture; assert per-eye view matrices now
  track the controller sim; undo restores. Then the real thing,
  in-headset, hand outstretched. Warn him it will be nauseating.
  That is not a bug.

**Traps:** EGL before XR, always (D.2's zombie scar). Session-state
transitions (READY→begin, STOPPING→end) exactly as
`probe/components/xr_session/xr_session.cpp` handles them — the guardian
and doffing arrive as state changes, not errors.

---

## Phase F — Real audio devices (PKG-8.1, FRZ-4.1, and the rung-8 close)

**Goal:** rung 8 → 18/18. Two halves, independent of E.

### F.1 Device machinery (the audio executor grows its device half)

| item | clause | what |
|---|---|---|
| `src/audio_dev/out_aaudio/` | machinery | Quest output: AAudio PCM_FLOAT stereo, 256-frame buffer, disconnect error_callback (a dead stream NEVER calls back — `probe/components/audio_output/audio_output_android.cpp:21` documents it; salvage the recovery shape from `audio_engine.hpp`'s `recover_if_dead`). |
| `src/audio_dev/out_host/` | machinery | host output (ALSA/SDL — take the probe's SDL backend for boredom). |
| `usb_watch` (native) | generated shell + machinery feed | observation source: device present/absent events (PKG-8's "observation nodes"). |

The `dac` node does not change — it is the passive boundary (that fight
was settled 2026-07-05; the dissolution marker moved to PKG-8.1 for
exactly this moment). The executor sums dacs into the stream, the Pd
model, one hardware owner.

### F.2 Witnesses

- **PKG-8.1:** host, USB DAC in hand: hot-plug → `usb_watch` emits →
  the audio package splices in as an ENGINE-GRAPH EDIT visible in the
  engine's op history (the criterion's whole point: environment
  observation is graph editing, and history shows it). No restart.
- **FRZ-4.1:** Quest, over the real mesh: wire a graph to the remote
  freeze node → Linux (advertising the toolchain capability) freezes →
  signed .so returns by hash under the MSH-5.1 plugin gate → hot-loads
  via the FRZ-1.1 swap path. Every piece already exists green in
  isolation (freezer, placement, plugin trust, render-swap); this
  criterion is their composition across two real devices.

When F closes: rung 8 is 18/18, and the runner's remaining pending is
rung 10 alone (skipped, Travis's call, unaffected by any of this).

---

## Appendix A — The complete authorized native delta

Machinery (7 components): `render/egl_surface`, `render/render_region`,
`host/window`, `xr/session`, `xr/frame_loop`, `audio_dev/out_aaudio`,
`audio_dev/out_host`. Trampolines (not natives): `shell_main`,
`quest_main`.

Natives (10): `draw`, `mesh_from_spans`, `surface_flat`, `glyph_quads`,
`pointer`, `key_text`, `head_pose`, `controller`, `usb_watch`, plus any
generator-stamped ONE-LINE leaves the gesture graphs prove necessary
(each recorded in STRENGTHENINGS.md when added).

Everything else in this plan is JSON under `graphs/` or datasets in the
store. If the work in front of you is not on this list and is not a
graph, re-read "The governing principle" before writing it.

## Appendix B — Blessing points and documentary moments

1. First triangle (Phase A) — PNG to Travis. 2. First hand-authored
graph in the windowed editor (Phase C) — his hands, log it. 3. First
greenfield boot on the Quest (Phase D). 4. The editor in-headset
(Phase E, PKG-3.1). 5. Eyeballs in his own hand (PKG-3.3) — the take
that names the repo. Never self-bless; he is the testimony.

## Appendix C — Standing discipline (unchanged, restated because Phase C will test it)

Commit per criterion, id in the message. Test and HARNESS.md row before
implementation. `STUCK.md` after a resisted session, then the next
criterion in the SAME phase. status.md resume block at every stop.
No `git push`. The four rules of BUILDER.md, especially rule 4 — this
entire plan is rule 4 with a schedule.
