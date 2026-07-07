# WI-7: tick_graph overhaul

## Summary
`tick_graph` currently calls `process()` in insertion order with no propagation.
Rewrite it to:
1. Topological sort nodes by edges (Kahn's algorithm)
2. Apply incoming edge values via typed setters
3. Call process
4. Collect outputs via `push_outputs`
5. Collect draw calls via `push_draw_calls`

## Work

### topo_sort helper (static function in signal_graph.cpp)
Kahn's algorithm: build in-degree map, BFS from zero-in-degree nodes.
If cycle detected, append remaining nodes in insertion order.

### tick_graph rewrite
```
g.draw_calls.clear()
order = topo_sort(g.nodes, g.edges)
for each idx in order:
    n = g.nodes[idx]
    // apply incoming edges
    for each edge E where E.to_node == n.id:
        key = E.from_node + "." + E.from_port
        if key in g.values:
            std::visit dispatch to set_scalar_in / set_vec3_in / set_vec4_in / etc.
    // process
    if n.desc->process: n.desc->process(n.data, time_s)
    // collect outputs
    if n.desc->push_outputs:
        build EyeballsOutputCtx with emit_* lambdas writing to g.values
        n.desc->push_outputs(n.data, &ctx)
    // collect draw calls
    if n.desc->push_draw_calls:
        DrawCallCtx ctx{n.id, &g.draw_calls}
        n.desc->push_draw_calls(n.data, &ctx)
```

`EyeballsOutputCtx` emit lambdas write `PortValue` variants into `g.values` keyed
as `"node_id.port_name"`.

### DrawCallCtx
Requires WI-1's `node_id` field to be present.

### Dependencies
Requires WI-1 (DrawCallCtx with node_id), WI-2–4 (nodes with outputs).
WI-6 (edge round-trip) must be done so edges exist to test propagation.

### Tests (signal_graph.test.cpp)
- Two nodes with edge: verify producer runs before consumer
- After tick_graph, verify g.values contains output key from producer
- Verify g.draw_calls populated if a DrawFn-output node is in graph

## Acceptance
- `tick_graph` does topo sort + propagation + output collection
- Tests pass
- `sh/build.sh` passes
- Commit
