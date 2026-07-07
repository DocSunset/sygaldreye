# store — commit, availability, refs (the store package's machinery)

Owning package: `store` (a complication; its face is vocabulary, this is
the machinery behind it). Allowed dependencies: `formats/*`.
Spec: ch. 3 — content-addressed objects (put/get, verification is
re-hashing); TWO commit paths (derivation: recipe, memoizable, evictable;
capture: testimony, provisional until provided); refs bind names to hashes
and every rebind APPENDS to the trail (undo = rebind to the predecessor);
provide/compatible availability (caches evict non-provided copies freely;
durability = someone provides it); chunked resumable fetch (only missing
chunks move on retry); back-links indexed at commit (STO-9). Peers here
are in-process stores — the mesh transport (rung 9) replaces the transfer
edge, never these semantics.
