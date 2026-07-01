# Zero-copy graph edges via stable output pointers

## Problem

Currently each edge incurs two copies per tick:
1. `push_outputs` copies node output field values into `Graph::values` (a `std::unordered_map<std::string, PortValue>`)
2. `set_*_in` copies those values back into downstream node input fields

## Design

Replace the value-copy approach with stable pointer wiring:

- Each output port field lives at a fixed address inside the node struct (struct members don't move).
- At graph construction / swap time, wire each edge as a `(void* src, void* dst, PortKind tag)` triple pointing directly into the upstream and downstream node structs.
- Each tick, instead of push/set round-trips through `Graph::values`, iterate the wired edges and memcpy (or assign) directly from src to dst using the type tag — one pass, no map lookups.

`Graph::values` can be removed or kept only for introspection/debug.

The C ABI is unaffected: `push_outputs` and `set_*_in` remain on the descriptor for backwards compatibility with dynamically loaded plugins. Internally wired (same-session) nodes use the pointer path; cross-ABI edges fall back to the existing path.

## Work

- Add a `WiredEdge { void* src; void* dst; PortKind tag; }` type to `signal_graph`.
- Add `port_schema` parsing at `parse_graph` time to resolve field offsets and populate `Graph::wired_edges`.
- Update `tick_graph` to iterate `wired_edges` instead of (or before) the map path.
- Update `push_outputs` / `set_*_in` on the descriptor to write to the struct fields directly (they already do — the pointer path just skips the map).
- Audit all existing nodes to confirm output fields are non-const value members (they are — `field.value` is mutable).
- Remove `Graph::values` map once all built-in edges use the pointer path; keep it only for plugin/ABI-fallback edges.

## Note: subgraph boundaries

Nodes own their outputs and take inputs by reference — no copies anywhere, including at subgraph boundaries. A subgraph node's inlets and outlets are pointers into its inner nodes' fields. The outer graph wires to those pointers the same way it wires any other edge. A subgraph node's `process` call is a `tick_graph` on the inner graph — inputs are already wired by reference before `process` runs, outputs are already owned by inner nodes and pointed to by outer edges. No extra work at the boundary.
