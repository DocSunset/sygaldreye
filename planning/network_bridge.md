# Network Bridge — how it works today, where it goes (walkthrough for Travis)

## Today (EXISTS, proven 2026-06-10)

Two nodes in `components/net_nodes`:

- `udp_send`: inputs `in` (float), `channel` (0–127), `port`. Every tick it
  datagram-sends `"<channel> <value>"` as ASCII to `127.0.0.1:<port>`.
- `udp_recv`: inputs `channel`, `port`; output `out` (float). Lazily binds
  a nonblocking socket (SO_REUSEADDR), drains all pending datagrams each
  tick, keeps the last value whose channel matches.

That's the whole protocol. The demo bridged two app instances: instance
A's lfo → `udp_send ch0`, instance B's `udp_recv ch0` → cube.pos_y. ~1
frame of latency. The boundary is invisible to the consuming node — B's
cube has no idea its driver lives in another process.

### Deliberate properties
- **Last-value-wins** (drain loop): matches the "value" edge kind — a
  slow consumer never backlogs.
- **Fire-and-forget**: no handshake, no acks. A dead receiver costs
  nothing; a dead sender just freezes the last value.
- **Channel numbers, not names**: because params couldn't hold strings
  when it was written. text params exist now — channels can become names.

### Known limits (all deliberate v1 cuts)
1. Loopback only — `host` needs a text param (now possible).
2. Scalars only — vectors/quats need either per-component channels (works
   today via split3/join3!) or a binary payload format.
3. One bind per port per process; two udp_recv on one port fight.
4. No discovery — mdns_advertiser exists but isn't graph-wired.
5. Events would be lossy (UDP) — fine for values, wrong for events.

## Where it goes (slice 4 in the vision)

The bridge becomes a **cross-region edge**, not a node pair you wire by
hand. Sketch:

1. **Transport**: one connection per peer pair, WebSocket (or raw TCP w/
   WS upgrade path) because the browser peer can ONLY speak WS. UDP stays
   as an optional fast lane for value-kind edges on native peers.
2. **Proxy nodes**: when a graph references a node that lives on another
   peer (`"host":"quest.local"` on the node entry), the local executor
   instantiates a *proxy* generated from the remote type's descriptor
   metadata (port schema travels as JSON — `port_schema_reader` already
   parses these). Inputs forwarded over the wire, outputs mirrored back.
   The node ABI's C-function-pointer design means a proxy is just another
   descriptor — the evaluator can't tell.
3. **Edge kinds map onto the wire**: value edges → latest-value frames
   (coalescable), event edges → reliable ordered messages, stream edges →
   sequenced chunks with a jitter buffer. This is why the edge taxonomy
   (edge_executor.design.md) comes first: the bridge inherits its
   semantics from edge kinds instead of inventing its own.
4. **Discovery**: mdns_advertiser wrapped as a source node emitting peer
   events; a `peer` registry the proxies resolve against.
5. **Plugin shipping rides the same pipe**: POST /plugins already accepts
   .so/.json; over the bridge it becomes "send node type to peer", which
   is the LLM-codegen loop's transport (slice 7).

### What I'd build first when you green-light slice 4
- text-param `host` on udp nodes (hours, no design risk) — cross-device
  values immediately, Quest⇄desktop.
- then stop extending UDP nodes and build the WS transport + proxy
  descriptor, because that's the real architecture and the UDP pair
  becomes a legacy convenience.

## Questions you'll probably ask
- *Security?* v1: trusted LAN only. Auth/encryption deferred deliberately.
- *NAT/internet?* Out of scope until a relay peer exists; LAN-first.
- *Clock sync?* Value edges don't need it; stream edges will (audio over
  network is a stretch goal, not slice 4).
