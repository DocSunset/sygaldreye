# WI-9: SSE broadcast on graph change

## Summary
After a successful POST /graph swap, broadcast an SSE `"graph"` event so connected
clients (browser, companion) know the graph changed.

## Work
In app.cpp's POST /graph handler, after `state.pending_graph_ = std::move(g);`:
```cpp
http_server.broadcast_event("graph", serialize_graph(*state.pending_graph_));
```

NOTE: The broadcast must happen inside the mutex lock (pending_graph_ is valid there).
Or serialize before moving:
```cpp
auto json = serialize_graph(*g);
std::lock_guard<std::mutex> lock(state.graph_mutex_);
state.pending_graph_ = std::move(g);
// broadcast outside lock (capture by value)
```

Also broadcast on the atomic frame-boundary swap (in the frame loop):
```cpp
{
    std::lock_guard<std::mutex> lock(state.graph_mutex_);
    if (state.pending_graph_) {
        state.active_graph_ = std::move(state.pending_graph_);
        state.vr_editor_.on_graph_changed(state.active_graph_.get());
        // broadcast_event here if http_server accessible, or queue it
    }
}
```

The simpler approach: broadcast at POST time (pre-swap) since the JSON is the same.

Verify `broadcast_event` is declared in `http_server.hpp`.

## Acceptance
- POST /graph broadcasts SSE "graph" event with new graph JSON
- `sh/build.sh` passes
- Commit
