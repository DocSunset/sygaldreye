# Phase 9: Static Graph Compilation

## Goal

A runtime-authored graph can be "frozen" into a generated C++ translation unit that
encodes the topology and initial parameters directly — no dynamic dispatch, no `dlopen`,
no JSON parsing at runtime. The compiler can inline and optimize across node boundaries.
The companion's existing cross-compile pipeline handles the build step.

## What the generated code looks like

Given this runtime graph JSON:

```json
{
  "nodes": [
    { "id": "sky",   "type": "sky_dome",     "params": { "sun_elevation": 0.3 } },
    { "id": "water", "type": "water_surface", "params": { "wave_height": 5.0  } }
  ],
  "edges": [
    { "from": "sky.sun_dir", "to": "water.sun_dir" }
  ]
}
```

The code generator emits:

```cpp
// generated_graph.cpp  (do not edit — produced by freeze_graph)
#include "sky_dome.hpp"
#include "water_surface.hpp"

struct FrozenGraph {
    SkyDome      sky{};
    WaterSurface water{};

    void init() {
        from_json(sky,   R"({"sun_elevation":0.3})");
        from_json(water, R"({"wave_height":5.0})");
    }

    void process(double time) {
        sky(time);
        water.inputs.sun_dir.value = sky.outputs.sun_dir.value;  // compiled-in edge
        water(time);
    }
};
```

Properties:
- Zero function pointer indirection
- Zero JSON parsing at runtime
- Topological sort computed at code-generation time, not at runtime
- The compiler sees the full call graph: inlining and cross-node optimisation are possible
- The type of each port is known statically: no value store, no type erasure

## The TMP/constexpr angle

For graphs whose topology is expressed entirely in C++ template parameters, the compiler
can compute the evaluation order and wire port values at compile time:

```cpp
template<typename... Nodes>
struct StaticGraph { std::tuple<Nodes...> nodes; };

// Constexpr edge list drives topological sort at compile time
constexpr EdgeList edges = { {"sky", "sun_dir", "water", "sun_dir"} };
using MyGraph = StaticGraph<SkyDome, WaterSurface>;
```

C++20 constexpr algorithms can sort the node evaluation order at compile time given a
known edge list. This is an elaboration on the code-generation approach; the generated
`.cpp` already captures most of the benefit more simply.

## Provenance and reproducibility

Freezing a graph requires knowing the exact source of every node. The node metadata
carries a `provenance` field for this purpose. The code generator uses it to locate
and check out the right source before compiling.

### Provenance kinds

**Inline source** (LLM-generated / single-file nodes):

The source is stored verbatim in the graph JSON. No external lookup needed.

```json
{ "id": "custom", "type": "my_effect",
  "provenance": { "kind": "inline", "source": "// full .cpp text here" } }
```

**Nix flake reference** (library nodes with dependencies):

A pinned flake URL captures the full closure: source, transitive headers, compiler
version, flags. `nix build` with this reference reproduces the exact source tree,
anywhere, without trusting the URL to remain live (Nix binary caches handle
content-addressed storage).

```json
{ "id": "water", "type": "water_surface",
  "provenance": { "kind": "nix", "flake": "github:user/nodes/abc123def#water_surface" } }
```

**VCS reference** (simple versioned nodes):

```json
{ "provenance": { "kind": "git",
                  "url": "https://github.com/user/nodes",
                  "commit": "abc123def456",
                  "path": "water_surface/water_surface.cpp" } }
```

### Nix as the canonical form

Since this project already uses Nix, the Nix flake reference is the most rigorous
provenance kind. A library of curated nodes is a Nix flake with a `nodes` output —
a collection of derivations, each exposing both a compiled `.so` and the source needed
for static compilation. `nix flake update` pins new versions.

For inline/LLM-generated nodes, the source itself is the derivation — hash it for
a content-addressed identity if needed.

## "Freeze graph" workflow

```
User: "freeze the current graph"
    → companion calls GET /graph → gets JSON with full provenance
    → for each node:
        inline:  write source to temp file
        nix:     nix build <flake-ref>, get source path from store
        git:     git clone + checkout, get source path
    → code generator emits generated_graph.cpp + generated_graph.hpp
    → companion runs full CMake build (not just single-file compile)
    → produces optimised static APK or shared library
    → optionally POST to /plugins (the frozen graph as a single plugin)
```

## Key Design Decisions

**Code generation over pure TMP.** A generated `.cpp` is readable, debuggable, and
doesn't require the full graph topology to be expressible as C++ template parameters.
The constexpr TMP approach is an optional elaboration.

**Provenance is per-node, not per-graph.** A graph can mix inline nodes, Nix nodes,
and VCS nodes. The code generator handles each kind independently.

**The frozen graph is itself a plugin.** It satisfies `EYEBALLS_EXPORT_NODE` and can
be loaded back into the runtime graph via `/plugins`. This means a frozen scene can be
hot-swapped into a running session.

**JSON is compiled C++ that hasn't been compiled yet.** The runtime graph and the static
graph are the same thing at different stages. There is no impedance mismatch between
authoring at runtime and freezing to a static binary.

## Dependencies

Phase 4 (plugin ABI) — frozen graph satisfies the same ABI; provenance is additional
metadata on the descriptor.
Phase 5 (universal signal graph) — the graph JSON being frozen comes from GET /graph.
Phase 2 (networking) — companion fetches the graph and delivers the compiled result.
