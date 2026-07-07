# chunk — Merkle-DAG chunking

Owning package: `formats`. Allowed dependencies: `pins`, `cid`, `dagcbor`.
Spec: `architecture/14-formats-protocols.md` — bulk payloads above 256 KiB
(pinned) split into fixed 256 KiB raw chunks, listed by a dag-cbor index node
of links; simple, resumable, dedup-friendly (NAM-6.2).

- Inputs: a blob; an object sink (cid, bytes) supplied by the caller.
- Outputs: the root CID (the raw object itself, or the index node).
- Intended seams: the object directory (rung 6) is the durable sink; the
  conformance surface uses an in-memory one.
