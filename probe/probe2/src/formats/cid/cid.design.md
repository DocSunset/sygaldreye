# cid — content identifiers

Owning package: `formats`. Allowed dependencies: `pins`, BLAKE3 reference C
implementation (maturity import, ADR-033; flake-pinned, portable lane only —
deterministic everywhere).
Spec: `architecture/14-formats-protocols.md` — CIDv1, multibase base32
(lowercase, no padding), multicodec dag-cbor 0x71 / raw 0x55, multihash
blake3-256 0x1e. Fixture vectors: `conformance/fixtures/blake3-vectors.json`
(NAM-6.1).

- Inputs: bytes to hash; CID text/bytes to parse or verify.
- Outputs: CID bytes and text; verification verdicts (re-hash equality).
- Intended seams: the object directory (NAM-6.2/STO) and the verified fetch
  path key everything by these.
