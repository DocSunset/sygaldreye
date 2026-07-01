# WI-6: Edge round-trip in JSON

## Summary
`parse_graph` ignores the `"edges"` array; `serialize_graph` emits `"edges":[]`.
Fix both so edges round-trip through JSON.

## Work

### parse_graph (signal_graph.cpp)
After parsing nodes, find the `"edges"` array and parse each object:
```
{ "from": "sky.sun_elevation_out", "to": "water.sun_elevation" }
→ Edge{ from_node="sky", from_port="sun_elevation_out",
         to_node="water",   to_port="sun_elevation" }
```
Split `"from"` value on first `.` to get `from_node` and `from_port`.
Split `"to"` value on first `.` to get `to_node` and `to_port`.

### serialize_graph (signal_graph.cpp)
Replace `out += "],\"edges\":[]}";` with actual edges:
```cpp
out += "],\"edges\":[";
bool first_edge = true;
for (const auto& e : g.edges) {
    if (!first_edge) out += ',';
    first_edge = false;
    out += "{\"from\":\""; out += e.from_node; out += '.'; out += e.from_port;
    out += "\",\"to\":\""; out += e.to_node; out += '.'; out += e.to_port;
    out += "\"}";
}
out += "]}";
```

### signal_graph.test.cpp
Add test: parse JSON with edges, then serialize_graph and verify edge appears in output.

## Acceptance
- `parse_graph` populates `g->edges`
- `serialize_graph` emits actual edges
- Test passes
- `sh/build.sh` passes
- Commit
