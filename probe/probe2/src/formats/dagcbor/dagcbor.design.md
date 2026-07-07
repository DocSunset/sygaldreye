# dagcbor — the canonical encoder

Owning package: `formats`. Allowed dependencies: `nlohmann_json`.
Spec: `architecture/14-formats-protocols.md` (ADR-017); executable oracle
`conformance/reference/dagcbor.py` (FMT-1) — match byte-for-byte, never port.

- Inputs: a JSON-projection value (in-memory `nlohmann::json`; DAG-JSON
  conventions per `conformance/HARNESS.md`).
- Outputs: canonical dag-cbor bytes — the only writer of at-rest bytes.
- Intended seams: the low-level `writer` primitives are the emit target for
  generated per-kind codecs (ABI-1, AUT-3).
