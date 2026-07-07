# WI-3: DataSourceNode Template for xr_sources

## Goal

Collapse the three structurally identical input-source nodes in `xr_sources.hpp`
(`HeadPoseNode`, `LeftControllerNode`, `RightControllerNode`) using a common
`DataSourceNode<Outputs>` CRTP base or a shared mixin:

```cpp
template<typename Derived>
struct DataSourceNode {
    void operator()(double) {}  // no-op: driven externally via set_state()
};
```

The goal is to eliminate the duplicated `operator()(double) {}` no-op and the
copy-pasted port-schema comments. The `set_state()` methods differ per node
(different XrPosef fields) so they stay in each derived class.

## Acceptance Criteria

- `xr_sources.hpp` uses the shared base for all three nodes
- No functional change — all ports and set_state() APIs remain identical
- `sh/build.sh` passes
- `integration_real_nodes` tests still pass

## Notes

This is a cosmetic refactor with very low risk. No new component directory is needed;
the template can live in `xr_sources.hpp` or a new `components/data_source_node/`
header if it's likely to be reused for other external-state injectors (e.g. mic capture,
OSC inputs).
