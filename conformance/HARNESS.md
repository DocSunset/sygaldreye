# The harness contract

The conformance tests drive the implementation through a single binary,
`./syg`, at the repo root (a symlink into your build tree is fine). Each
rung adds subcommands; a subcommand reads stdin, writes stdout, exits
nonzero on error with a message on stderr. The tests are ALREADY WRITTEN
for the early rungs — your job is to make `./syg` agree with the reference
oracles in `conformance/reference/` (which are the executable spec; never
import or port them into the implementation).

| rung | subcommand | contract |
|---|---|---|
| 1 | `syg encode` | JSON value on stdin → canonical dag-cbor bytes on stdout (oracle: reference/dagcbor.py) |
| 1 | `syg parse-address` | address text on stdin → one-line JSON `{"kind","head","route"}` (oracle: reference/address.py) |
| 3 | `syg replay-tape` | boot tape on stdin → JSON `{nodes, edges, defaults}` of the built graph (oracle: reference/tape.py) |
| 2 | `syg render-movement <fixture> <seconds>` | render a frozen movement headless → raw float32 mono on stdout (checked against fixtures/golden-audio.md properties) |
| 1 | `syg pins` | no stdin → one-line JSON of the frozen ch. 14 pins (multicodec/multihash numbers, multibase, chunk size, escape set, tape records, edit ops, wire kinds) — the FMT-5 freeze surface |
| 4+ | added per rung when its first test is written; record the contract here in the same commit (FMT-5 discipline applies) |

The JSON used on these surfaces is the projection, not the canonical form —
the bytes that matter are the ones `syg encode` emits. The projection uses
the DAG-JSON conventions for what JSON cannot carry natively: a byte string
is `{"/": {"bytes": "<base64, standard alphabet, no padding>"}}`, a link is
`{"/": "<cid text>"}`, and the map key `"/"` is reserved — an ordinary map
may not use it (recorded 2026-07-05 when the FMT-1 differential test first
carried bytes; `_helpers.to_projection` is the test-side half).
