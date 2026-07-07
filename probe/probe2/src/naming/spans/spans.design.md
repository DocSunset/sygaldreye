# spans — sequence-kind traversal

Owning package: `naming`. Allowed dependencies: `environment`.
Spec: ch. 2 (NAM-7) — sequence kinds traverse through persistent pieces;
links attach to hashes, positions are derived per version, so links survive
editing and versions share structure.

- A `text` node is `{"kind":"text","pieces":[<links to raw string pieces>]}`.
- Inputs: a text node; an absolute position (to mint a span) or a span
  `{piece, start, len}` (to locate it in a version).
- Outputs: the span's characters and its derived position in that version —
  the version map, computed on demand.
- Intended seams: piece granularity (lines today) generalizes to enfilade
  descendants (ropes / persistent trees) without changing the span shape.
