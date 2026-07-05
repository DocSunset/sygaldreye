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
| 1 | `syg hash` | raw bytes on stdin → CID text (raw multicodec) on stdout (oracle: reference/cid.py + fixtures/blake3-vectors.json) |
| 1 | `syg verify <cid>` | raw bytes on stdin → `{"ok":bool}` — re-hash equality against the given CID (NAM-6.1) |
| 1 | `syg chunk-put` | JSON `{"blobs":[<bytes projections>]}` → `{"roots":[cid...],"objects":N,"stored_bytes":M}` — pinned 256 KiB chunking into a deduping object directory (NAM-6.2) |
| 1 | `syg naming` | scripted resolution session (NAM-1..5): JSON `{"objects":{name:projection},"refs":{refname:"$name"},"ops":[...]}` — objects commit in dependency order (`"$name"` inside link escapes and addresses substitutes the committed cid); ops are `resolve`/`normalize` (→ `{"fixity","normalized","value","io"}` / `{"normalized"}`), `subscribe {addr,as}`, `move-ref {ref,to}`, `events` (drain deliveries), `span {of,at:[pos,len]}` / `span-text {of,span}` (NAM-7 sequence traversal) → `{"cids":{name:cid},"results":[...]}` |
| 1 | `syg connection-legal` | JSON `{"from":[kind,discipline],"to":[kind,discipline]}` → `{"legal":bool,"mapping":name-or-null}` — the NAM-5 promise oracle |
| 2 | `syg tick-audit` | tick the frozen hello-cosine movement with naming instrumentation live → `{"ticks":N,"lookups":M}`; NAM-5.4 requires M=0 |
| 2 | `syg codec-selftest <type>` | JSON in-port map on stdin → round-trip through the GENERATED C++ codec (ABI-1.1); undeclared fields error |
| 2 | `syg hook-audit` | full hook table over generated shells, 10,000 callbacks with live param edits → `{"callbacks","counts":{phase:{allocs,locks}}}` (ABI-2.1) |
| 2 | `syg create-audit good\|bad` | resource acquisition timing: prepare acquires (ok) vs create acquires (allocation-discipline abort) (ABI-2.2) |
| 2 | `syg fault-audit` | declared-fallible shell converts a thrown exception to a fault record; region keeps ticking → `{"faults","ticks","ticks_after_fault"}` (ABI-3.1) |
| 2 | `syg quarantine-audit` | undeclared throw in a quarantined (subprocess) plan: deaths, wired restart ladder, testimony naming the route (ABI-3.2) |
| 3 | `syg render-tape <seconds>` | boot tape on stdin → crown builds the plan at runtime → raw float32 mono on stdout; byte-identical to the frozen movement (COR-2 ladder start) |
| 4 | `syg roundtrip` | composite-graph interchange JSON on stdin → parse → serialize → stdout; identity on the persisted surface, derived structure never emitted (LNG-8.1) |
| 4 | `syg resolve-hash <cid> <objdir>` | verified hash→bytes from an on-disk object directory; corrupt = loud error, miss = clean miss (SZ-3.1, the debugger of last resort) |
| 4 | `syg palette` | the registry-face organ: linked native names as data; equals the generated registration manifest (SZ-2) |
| 4 | `syg swap-audit <tape2> <seconds>` | tape on stdin → render seconds/2 → slot-swap to tape2 (state migrates by route) → render seconds/2; raw float32 (EXE-5) |
| 1 | `syg pins` | no stdin → one-line JSON of the frozen ch. 14 pins (multicodec/multihash numbers, multibase, chunk size, escape set, tape records, edit ops, wire kinds) — the FMT-5 freeze surface |
| 4+ | added per rung when its first test is written; record the contract here in the same commit (FMT-5 discipline applies) |

The JSON used on these surfaces is the projection, not the canonical form —
the bytes that matter are the ones `syg encode` emits. The projection uses
the DAG-JSON conventions for what JSON cannot carry natively: a byte string
is `{"/": {"bytes": "<base64, standard alphabet, no padding>"}}`, a link is
`{"/": "<cid text>"}`, and the map key `"/"` is reserved — an ordinary map
may not use it (recorded 2026-07-05 when the FMT-1 differential test first
carried bytes; `_helpers.to_projection` is the test-side half).
