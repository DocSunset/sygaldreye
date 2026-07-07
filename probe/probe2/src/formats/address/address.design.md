# address — the address grammar

Owning package: `formats`. Allowed dependencies: none (std only).
Spec: `architecture/14-formats-protocols.md` (ADR-007/029); executable oracle
`conformance/reference/address.py` (FMT-2 / NAM-1.1) — match exactly.

- Inputs: address text (one of the three first-step spellings).
- Outputs: `(kind, head, route steps)` — percent-unescaped local names.
- Intended seams: the resolver (NAM-1/2) walks the parsed route from its
  environment node; print joins it when serialization is demanded.
