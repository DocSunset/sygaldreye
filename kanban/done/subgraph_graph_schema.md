# Subgraph: extend Graph schema for inlets and outlets

See `planning/subgraph.md` for full context.

## What to do

Add inlet/outlet metadata to `Graph` and extend `parse_graph` /
`serialize_graph` to handle them.

### 1. New types in `signal_graph.hpp`

```cpp
struct InletDecl  { std::string name, node, port; };
struct OutletDecl { std::string name, node, port; };
```

Add to `Graph`:

```cpp
std::vector<InletDecl>  inlets;
std::vector<OutletDecl> outlets;
```

### 2. `parse_graph` in `signal_graph.cpp`

After parsing `"edges"`, parse optional `"inlets"` and `"outlets"` arrays.
Each element is an object with string fields `"name"`, `"node"`, `"port"`.
Use the existing `find_string` / `split_array_objects` helpers — no new JSON
machinery needed.

### 3. `serialize_graph` in `signal_graph.cpp`

After the `"edges"` array, emit `"inlets"` and `"outlets"` arrays only when
non-empty:

```json
"inlets":[{"name":"gain","node":"amp","port":"level"}],
"outlets":[{"name":"render","node":"cube","port":"render"}]
```

### 4. Tests in `signal_graph.test.cpp`

Add tests:
- A graph JSON with `"inlets"` and `"outlets"` round-trips correctly through
  `parse_graph` → `serialize_graph`.
- A graph JSON without those keys parses without error (they default to empty).

## Constraints

- No changes to `EyeballsNodeDescriptor` or `ComponentRegistry` in this item.
- No changes to `tick_graph` in this item.
- Each `.cpp` file should stay under ~100 lines of substantive code; split if
  needed.
- Treat all compiler warnings as errors.
