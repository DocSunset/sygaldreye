# WI-1: draw_call port + DrawCallCtx node_id

## Summary
Phase 12 added `DrawFn` and `DrawCallCtx` but `DrawCallCtx` lacks a `node_id` field.
The spec requires `DrawCallCtx` to carry `node_id` so `tick_graph` can tag draw calls
by their source node.

## Work
In `components/eyeballs_node_abi/eyeballs_node_abi.hpp`, add `node_id` to `DrawCallCtx`:

```cpp
struct DrawCallCtx {
    std::string         node_id;
    std::vector<DrawFn>* calls;   // points into Graph::draw_calls
};
```

Verify `make_descriptor<T>()`'s `push_draw_calls` lambda compiles and correctly accesses
`ctx->calls` via `static_cast<DrawCallCtx*>(ctx_ptr)`.

## Acceptance
- `sh/build.sh` passes
- `DrawCallCtx` has both `node_id` and `calls` fields
- Commit
