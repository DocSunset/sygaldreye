# naming — the resolution engine package

Ch. 2's deliverables: the resolution engine (`resolver`), the environment it
stands on (`environment`: object store, refs, subscriptions — ADR-029's HERE),
and the promise oracle (`oracle`). The address grammar lives in `formats`.
Allowed package dependencies: `formats`, nlohmann_json.

Committed objects are held in-motion as JSON projections beside their
canonical identity (hash-and-encode happens at commit, never on read —
ADR-017); reads are counted I/O once and memoized after (NAM-2.1).
