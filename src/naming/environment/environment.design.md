# environment — the resolver's HERE

Owning package: `naming`. Allowed dependencies: `formats/{cid,dagcbor,pins}`.
Spec: ch. 2 + ADR-029 — resolution begins at the resolver's environment node:
its object store, its ref table, (later) its peer table and petnames.

- Inputs: committed projections; ref binds/moves; subscriptions.
- Outputs: counted, memoized object reads (`io_count` — NAM-2.1's counter);
  ref-move events, delivered exactly once per live-address subscriber
  (NAM-2.2), never polled.
- Intended seams: the backing map is where the durable object directory
  (rung 6) and the verified mesh fetch (rung 9) splice in.
