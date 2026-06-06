# Phase 9 — Static Graph Compilation Workflow

## End-to-end steps

1. Author a graph in VR or via `POST /graph` with JSON.
2. Compile and upload the frozen graph to the headset:
   ```
   python companion/compile_frozen.py \
       --headset http://quest.local:8080 \
       --ndk $ANDROID_NDK_ROOT \
       --repo .
   ```
   Or use the companion's `--freeze` flag:
   ```
   python companion/companion.py --host quest.local --freeze
   ```
3. The headset hot-loads the `.so` as a new node type `"frozen_graph"`.
4. Activate the frozen graph:
   ```
   POST /graph {"nodes":[{"id":"fg","type":"frozen_graph","params":{}}],"edges":[]}
   ```
5. The frozen graph runs with zero dynamic dispatch — all nodes are direct struct members.

## How it works

`freeze_graph.py` fetches `GET /graph` and generates `frozen_graph.cpp`:
- Each node becomes a typed struct member (e.g. `SkyDome sky{};`).
- `operator()(double time)` calls each member in JSON order.
- Edges are documented as comments; live port wiring is a future enhancement.
- `EYEBALLS_EXPORT_NODE(FrozenGraph)` satisfies the plugin ABI.

`compile_frozen.py` cross-compiles with the NDK aarch64 toolchain and POSTs
the resulting `.so` to `/plugins`.

## Provenance kinds

| Kind | How generator finds the header |
|------|-------------------------------|
| `local` | Uses `provenance.header` path from graph JSON (set by `serialize_graph`) |
| `inline` | Same as `local` |
| `nix` | Not yet implemented (logs a warning, skips node) |
| `git` | Not yet implemented (logs a warning, skips node) |
