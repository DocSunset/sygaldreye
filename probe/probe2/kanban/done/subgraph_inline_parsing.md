# Subgraph: inline subgraph definitions in parse_graph

See `planning/subgraph.md` for full context.

**Depends on:** `subgraph_node_impl` (SubgraphDescriptor exists).
Independent of `subgraph_registry_loading`.

## What to do

Extend `parse_graph` in `signal_graph.cpp` to handle nodes whose type is not
in the registry but whose JSON object contains a `"graph"` key. When found,
parse the value of `"graph"` as an inline subgraph, build a transient
`SubgraphDescriptor`, register it under a generated type name, and proceed.

### JSON format

```json
{
  "nodes": [
    {
      "id": "reverb",
      "type": "__inline__",
      "graph": {
        "inlets":  [{"name": "audio_in", "node": "delay", "port": "input"}],
        "outlets": [{"name": "audio_out", "node": "mix",   "port": "output"}],
        "nodes": [...],
        "edges": [...]
      }
    }
  ],
  "edges": [...]
}
```

The `"type"` field for inline nodes can be anything (or omitted); presence of
`"graph"` is the trigger. The generated type name is `"__inline_<node_id>"`.

### Changes to `parse_graph`

`parse_graph` currently returns `nullptr` when `registry.find(type)` fails.
Add a prior check: if the node JSON object contains a `"graph"` key, extract
its value as a JSON substring, recursively call `parse_graph(inner_json,
registry)`, and if successful, construct a `SubgraphDescriptor` from it.

Because `parse_graph` only has a `const ComponentRegistry&`, it cannot mutate
the registry. Instead, keep a local `std::vector<std::unique_ptr<SubgraphDescriptor>>
inline_descriptors` within `parse_graph` and transfer ownership into the
returned `Graph` via a new field:

```cpp
// in Graph (signal_graph.hpp):
std::vector<std::unique_ptr<SubgraphDescriptor>> owned_descriptors;
```

The `NodeInstance.desc` pointer into an inline descriptor remains valid as
long as the `Graph` is alive (same object), satisfying the lifetime
requirement.

### Helper: extract nested JSON object

The existing `find_object(json, key)` helper locates `"key":{...}` and returns
the braced substring — use it to extract `"graph":{...}` from the node object.

### Tests in `signal_graph.test.cpp`

- A graph JSON with one inline subgraph node parses successfully; the node's
  `desc->type_name` begins with `"__inline_"`.
- An inline subgraph with one inlet and one outlet correctly propagates a
  scalar value end-to-end through a `tick_graph` call.
- An inline subgraph nested inside another inline subgraph parses and ticks
  correctly.

## Constraints

- `parse_graph` signature is unchanged.
- `const ComponentRegistry&` is still const — no mutation.
- Inline descriptors are owned by the `Graph` they belong to; they do not
  outlive it.
- Treat all warnings as errors.
