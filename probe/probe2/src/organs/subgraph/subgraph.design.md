# subgraph — clone-behind-trampolines composition

Owning package: `organs`. Clause: **machinery** (core-named). Allowed
dependencies: `parser`, `registry_face`.
Spec: LNG-6 — a template graph (a `.json` in graphs_dir IS a plugin)
registers as a node type; spawning clones it: inner nodes namespaced
`outer.inner`, outer edges rewired through the declared inlets/outlets,
the node entry's params landing as the inner inlets' defaults
(creation-args). Expansion is a doc→doc transform, recursive with a cycle
guard; resource-holder-ness is inferred from the interior (LNG-6.2).
