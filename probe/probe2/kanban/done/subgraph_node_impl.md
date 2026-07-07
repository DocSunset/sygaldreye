# Subgraph: implement SubgraphNode and SubgraphDescriptor

See `planning/subgraph.md` for full context.

**Depends on:** `subgraph_graph_schema` (InletDecl/OutletDecl in Graph).

## What to do

Create a new component `subgraph_node` that implements the subgraph node type
and the machinery to create an `EyeballsNodeDescriptor` for it dynamically.

### Runtime model

`SubgraphNode` holds:
- `std::unique_ptr<Graph> inner_` — the inner graph
- `std::unordered_map<std::string, PortValue> inlet_cache_` — values received
  from the parent graph before `operator()` is called, keyed by inlet name

`operator()(double t)`:
1. For each `InletDecl` in `inner_->inlets`, look up `inlet_cache_[decl.name]`.
   Find the target node in `inner_->nodes` by `decl.node`. Call its
   `desc->set_*_in(data, decl.port.c_str(), ...)` dispatching on the
   `PortValue` variant. (Same dispatch pattern as in `tick_graph`.)
2. Call `tick_graph(*inner_, t)`.

`push_outputs(EyeballsOutputCtx* ctx)`:
- For each `OutletDecl`, look up `inner_->values["decl.node + '.' + decl.port"]`
  and emit it via the appropriate `ctx->emit_*` function.

`push_draw_calls(DrawCallCtx* ctx)`:
- Append all entries of `inner_->draw_calls` into `ctx->calls`.

### SubgraphDescriptor

A C++ struct that owns a dynamically-allocated `EyeballsNodeDescriptor` and
all heap strings it references (type_name, port_schema). Constructed from a
`Graph` and a type name string.

```cpp
struct SubgraphDescriptor {
    explicit SubgraphDescriptor(std::unique_ptr<Graph> graph_template,
                                std::string type_name);

    const EyeballsNodeDescriptor* descriptor() const;

private:
    std::unique_ptr<Graph>    graph_template_;  // cloned per create()
    std::string               type_name_;
    std::string               port_schema_;
    EyeballsNodeDescriptor    desc_;
};
```

`create()`: deep-clone `graph_template_` into a new `SubgraphNode` (the
`SubgraphNode` owns its `Graph`). Return `new SubgraphNode(...)`.

`destroy()`: `delete static_cast<SubgraphNode*>(p)`.

`process()`: `(*static_cast<SubgraphNode*>(p))(t)`.

`push_outputs()`: dispatch as above.

`push_draw_calls()`: dispatch as above.

`set_*_in()`: store value in `SubgraphNode::inlet_cache_` keyed by port name.

`port_schema_`: build a JSON string from the Graph's `inlets` (inputs) and
`outlets` (outputs) with `"kind":"unknown"` for each (type inference is future
work). Store in `port_schema_` and point `desc_.port_schema` at it.

### Graph deep-clone

`SubgraphDescriptor::create()` must produce an independent `SubgraphNode` from
the template, so add a `clone_graph(const Graph&) -> std::unique_ptr<Graph>`
free function (in `signal_graph.hpp`/`.cpp`) that re-instantiates each node
via `desc->create()` + `desc->deserialize(serialize(...))` and copies edges,
inlets, outlets.

### Files

```
components/subgraph_node/
    subgraph_node.hpp
    subgraph_node.cpp
    subgraph_node.test.cpp
    CMakeLists.txt
```

### Tests

- A `SubgraphNode` wrapping a simple inner graph (e.g. one node with one
  scalar input and one scalar output) correctly propagates an inlet value
  through to the outlet after `operator()()`.
- `push_draw_calls` forwards inner draw calls to the parent context.
- `clone_graph` produces an independent copy (mutating one does not affect the
  other).

## Constraints

- The `SubgraphDescriptor` owns its descriptor memory; no static storage.
- No changes to `tick_graph`, `EyeballsNodeDescriptor`, or `ComponentRegistry`
  in this item.
- Each `.cpp` file under ~100 lines of substantive code.
- Treat all warnings as errors.
