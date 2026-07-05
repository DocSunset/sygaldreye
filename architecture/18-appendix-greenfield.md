# Appendix — The greenfield build order

*The dependency order for building sygaldreye from an empty repository. Each
rung's gate is its slice of the conformance suite (ch. 17); no rung starts
until the previous rung's gate is green. The probe (`components/`, `app/`)
is a deprecated design probe: mined for kernels, tests, and golden outputs;
never migrated.*

1. **Formats first** (ch. 14): the generator (declarations → codecs +
   descriptors + bindings), dag-cbor profile, address grammar, op records,
   tape format. Gate: FMT-1..3 property tests in C++ and one host binding.
2. **The escapement** (chs. 13, 16): node contract + tick over a constexpr
   movement. Gate: COR-1; a hand-frozen hello-cosine movement produces the
   golden output headless (movement-level conformance is born here).
3. **The crown**: mutable plan + applier + tape replay. Gate: COR-2's
   ladder start — a tape builds hello-cosine at runtime; FMT-3.
4. **Liveness organs as vocabulary** (ch. 16 stratum 3): parser, codec
   nodes, naive resolver, registry-face, ref, subgraph, slot, reflection,
   the query four. Gate: SZ-7 (boot from embedded data to a live engine
   graph), EXE-5 (swap+migrate), LNG-8 (round-trip).
5. **Execution semantics** (chs. 4, 15): disciplines, region inference,
   the seven mappings, per-sample islands, quiescence. Gate: EXE-2/3/4/
   10/11, TCF-1/2, golden-audio hello-cosine with live edits.
6. **The store** (ch. 3): object machinery + store-graph face + commit
   paths + refs + op-tree history. Gate: STO-1..9, ADR-018 undo tests.
7. **Compile & realize** (ch. 5): engine v0 as data, deterministic compile
   + map, projection editing, interpret backend. Gate: CMP-1..7.
8. **Packages, in probe-proven order** (ch. 7): audio first (kernels
   salvaged from synth_core — they survive verbatim), then render, worker,
   then the codegen backend (freezer: FRZ-1..3 — movements now
   machine-frozen, closing the loop with rung 2), then XR.
9. **The mesh** (ch. 8, 14): keys/pairing, advertisement, fetch,
   ops-to-arbiter, placement. Gate: MSH-1..8, FMT-4 transcripts, PKG-6/7;
   peer-level conformance is born here.
10. **Hosts** (ADR-019): CPython module, browser/wasm. Gate: ABI-5,
    python-executes-graph notebook test.
11. **Surfaces**: editor patches, store browser, documents. Gate: EDR-1..8.
12. **The self-hosting closure** (ch. 17): the suite as datasets, the
    candidate-as-peer harness, sygaldreye-N derives N+1. Gate: CNF-1..5 —
    at which point the book, the suite, and the system are one object and
    the probe can be archived with honors.

**Salvage list from the probe**: synth_core kernels (verbatim — AUT-1
already enforced), the ugen/lift/editor test suites (become golden corpus
seeds and blessing candidates), editor.json + card.json (the editor's
decomposed form re-expressed in new topology format), the HRTF/audio device
machinery (audio package machinery), scene/demo graphs (conformance
corpus). Everything else is reference reading.
