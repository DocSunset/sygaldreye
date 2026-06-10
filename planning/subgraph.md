# Subgraph Planning Document

## Goal

Introduce subgraphs: a node whose implementation is itself a `Graph`. Analogous
to a Pure Data subpatch/abstraction. The main graph is just the subgraph loaded
first. Subgraphs compose: a subgraph may contain other subgraphs.

## Design decisions

### Inlets and outlets are graph-level metadata, not nodes

The external interface of a subgraph is declared as top-level `"inlets"` and
`"outlets"` arrays in the graph JSON. Each entry names an internal `node`+`port`
pair and gives it an external `name`. The type of each external port is inferred
from the type of the mapped internal port.

```json
{
  "inlets":  [{"name": "gain",   "node": "amp",  "port": "level"}],
  "outlets": [{"name": "render", "node": "cube", "port": "render"}],
  "nodes": [...],
  "edges": [...]
}
```

No passthrough node needed. The subgraph node's `operator()` injects inlet
values directly into the internal nodes before ticking the inner graph, then
reads outlet values out of the inner graph's value store afterward.

### A subgraph is an ABI-compliant node

`SubgraphNode` satisfies the same `EyeballsNodeDescriptor` contract as any
other node. `tick_graph` calls its `process` function pointer the same as any
other node. No special-casing in the evaluator.

The `SubgraphNode` holds:
- the inner `Graph`
- inlet/outlet mappings (vectors of `InletDecl`/`OutletDecl`)
- a `PortValue` cache keyed by inlet name (values injected from the parent
  graph before `operator()` is called)

Because each subgraph definition produces a distinct set of ports, each
subgraph type gets its own dynamically-allocated `EyeballsNodeDescriptor`.
A `SubgraphDescriptor` C++ object owns the descriptor and all heap strings.

### By-reference loading

`ComponentRegistry::load_plugin` detects `.json` extension and calls a new
`load_subgraph_json(path)` path instead of `dlopen`. It parses the file as a
graph, builds a `SubgraphDescriptor`, and registers it. The type name is
derived from the file stem (e.g. `reverb.json` → type name `"reverb"`).

### Inline subgraph

When `parse_graph` encounters a node whose type is not in the registry but
whose JSON object contains a `"graph"` key, it parses the value of `"graph"`
as an inline subgraph definition, builds a transient `SubgraphDescriptor`, and
registers it under a generated type name (e.g. `"__inline_<node_id>"`). This
makes inline and by-reference subgraphs share the same runtime representation.

### tick_graph is unchanged

`tick_graph` iterates nodes in topo order, applies edge values, calls
`desc->process`, collects outputs. The subgraph node's `process` function does
the inject → inner tick → (outlets are emitted in push_outputs). No changes to
the evaluator.

## File map

| Component | Files |
|---|---|
| Graph schema extension | `components/signal_graph/signal_graph.hpp`, `.cpp` |
| SubgraphNode + descriptor | `components/subgraph_node/` (new) |
| Registry .json loading | `components/component_registry/component_registry.hpp`, `.cpp` |
| Inline parsing | `components/signal_graph/signal_graph.cpp` |

## Implementation order

1. **signal_graph schema** — add `InletDecl`/`OutletDecl` to `Graph`; extend
   `parse_graph` and `serialize_graph`.
2. **subgraph_node component** — `SubgraphNode` + `SubgraphDescriptor`;
   tests. Depends on (1).
3. **registry .json loading** — `ComponentRegistry::load_subgraph_json` +
   `load_plugin` dispatch. Depends on (2).
4. **inline subgraph parsing** — extend `parse_graph` to handle inline
   `"graph"` key. Depends on (2).

Items (3) and (4) are independent of each other and can proceed in parallel
after (2).

## Key invariants

- A subgraph's `operator()` must call `tick_graph` on the inner graph, which
  recursively handles any nested subgraphs. No depth limit needed.
- Inlet injection uses the same `desc->set_*_in` mechanism tick_graph uses
  for ordinary edges. No new injection path.
- Outlet collection uses the inner graph's `values` map directly (keyed
  `"node_id.port_name"`), avoiding a second pass.
- Draw calls from the inner graph must be forwarded to the parent graph's
  `draw_calls` vector. The subgraph node's `push_draw_calls` appends inner
  `g.draw_calls` to the parent context.
- The `SubgraphDescriptor` must outlive all `SubgraphNode` instances created
  from it. The registry owns descriptors for by-reference subgraphs; for inline
  subgraphs, ownership transfers to the `Graph` that contains the node.
