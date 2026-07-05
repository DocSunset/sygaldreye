# generator — one declaration, everything else derived

Owning package: `generator` (a build-time tool, not a node).
Allowed dependencies: `authoring`, boost.pfr, nlohmann_json, the declared
endpoints headers under `src/nodes`.
Spec: ADR-017/019, AUT-3, ABI-1 — the keystone. It walks each registered
endpoints struct by reflection (PFR: real named fields) and emits, into
`<build>/generated/`:

- `<type>.descriptor.json` — ports with their (kind, discipline) promises;
- `codecs.hpp` — per-type `to_projection` / `from_projection` (in-ports:
  the settable, at-rest face); the ONLY writer besides the canonical
  encoder these feed;
- `bindings.py` — dataclasses + a PORTS table: bindings that cannot rot.

Temporal coupling: runs before `syg` compiles (syg includes codecs.hpp).
Intended seams: shells (ABI-2/3) and registration TUs (SZ-2) are emitted
here as later rungs demand; new types register in `generate.cpp`'s one
list.
