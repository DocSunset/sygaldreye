# src — the greenfield implementation

Empty on purpose until rung 2. What goes where (the runner tells you when):

- `escapement/` — the node contract and the tick loop. Nothing else, ever.
  No `#ifdef`, no allocation policy beyond the target's pool, no vocabulary.
- `crown/` — the mutable plan, the op applier, the tape reader.
- `organs/` — the liveness complications (parser, codec, resolver,
  registry-face, slot, subgraph, reflection, query four, mappings). Each is
  a node like any node; each file declares its clause marker.
- `nodes/` — all other natives. EVERY file carries a clause marker comment
  on line 1: `// clause: machinery | floor | maturity | scaffolding`
  (COR-5.1 gate). If you can't name the clause, it's a graph — author it in
  a dataset, freeze it if slow (ADR-033).

Generated code (codecs, descriptors, shells, bindings) lands in a
`generated/` subdirectory and is never hand-edited (ABI-1).
