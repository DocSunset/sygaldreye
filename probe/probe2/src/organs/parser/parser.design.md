# parser — the interchange decode organ

Owning package: `organs`. Clause: **machinery** (crosses data↔machinery —
ch. 16 stratum 3; on the shrink list: a graph is equivalent to its ops, so
this may one day reduce to a projection decoder). Allowed dependencies:
nlohmann_json.
Spec: LNG-8 / STO-7 — the composite graph's single-file interchange form
(kind, lock, topology{nodes, edges, lift_key?}, defaults). parse∘serialize
is identity on the persisted surface; derived structure (mappings, regions,
islands) NEVER persists.
