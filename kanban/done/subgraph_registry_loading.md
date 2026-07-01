# Subgraph: load .json subgraph definitions via ComponentRegistry

See `planning/subgraph.md` for full context.

**Depends on:** `subgraph_node_impl` (SubgraphDescriptor exists).

## What to do

Extend `ComponentRegistry::load_plugin` to detect `.json` files by extension
and route them through a new `load_subgraph_json` path instead of `dlopen`.

### Changes to `component_registry.hpp`

Add:

```cpp
// Load a subgraph definition from a JSON file; returns false on failure.
bool load_subgraph_json(const std::string& path);
```

Add a new private container for owned descriptors:

```cpp
#include "subgraph_node.hpp"   // SubgraphDescriptor
...
std::vector<std::unique_ptr<SubgraphDescriptor>> subgraph_descriptors_;
```

### Changes to `component_registry.cpp`

In `load_plugin`, before the `dlopen` path, check the file extension:

```cpp
if (path.size() > 5 && path.substr(path.size() - 5) == ".json")
    return load_subgraph_json(path);
```

Implement `load_subgraph_json`:
1. Read the file into a `std::string`.
2. Call `parse_graph(json, *this)` — the graph may itself reference other
   already-registered types, so pass `*this` as the registry.
3. Derive the type name from the file stem: everything after the last `/` or
   `\`, strip the `.json` suffix.
4. Construct a `SubgraphDescriptor(std::move(graph), type_name)`.
5. Register `descriptor()->descriptor()` as a builtin entry. Push the
   `SubgraphDescriptor` into `subgraph_descriptors_` for ownership.
6. Log success/failure with the existing LOG/LOGE macros.

### Destructor

`subgraph_descriptors_` is destroyed automatically by `~ComponentRegistry()`
via unique_ptr; no extra teardown needed. Verify that `dlclose` entries in
`entries_` do not reference any descriptor pointer owned by
`subgraph_descriptors_` (they don't — those entries hold `handle = nullptr`).

### Tests in `component_registry.test.cpp`

- Write a minimal valid graph JSON to a temp file (e.g. `/tmp/test_sub.json`),
  call `load_plugin("/tmp/test_sub.json")`, assert it returns true and
  `find("test_sub")` is non-null.
- A malformed JSON returns false.

## Constraints

- The registry does not take ownership of descriptors returned by `.so`
  plugins (those are static); it does own `SubgraphDescriptor` objects.
- No changes to `tick_graph` or `parse_graph` in this item.
- Treat all warnings as errors.
