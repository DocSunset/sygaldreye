# Appendix — The greenfield build order

*The dependency order for building sygaldreye from an empty repository. Each
rung's gate is its slice of the conformance suite (ch. 17); no rung starts
until the previous rung's gate is green. The probe (siloed under `probe/`)
is a deprecated design probe: mined for kernels, tests, and golden outputs;
never migrated.*

1. **Formats first** (ch. 14): the generator (declarations to codecs +
   descriptors + bindings), dag-cbor profile, address grammar, op records,
   tape format. Gate: fmt.encoder_conformance, fmt.address_grammar, fmt.boot_tape property tests in C++ and one host binding.
2. **The escapement** (chs. 13, 16): node contract + tick over a constexpr
   movement. Gate: cor.escapement_austerity; a hand-frozen hello-cosine movement produces the
   golden output headless (movement-level conformance is born here).
3. **The crown**: mutable plan + applier + tape replay. Gate: cor.crown_sufficiency's
   ladder start — a tape builds hello-cosine at runtime; fmt.boot_tape.
4. **Liveness organs as vocabulary** (ch. 16 stratum 3): parser, codec
   nodes, naive resolver, registry-face, ref, subgraph, slot, reflection,
   the query four. Gate: sz.boot_without_store (boot from embedded data to a live engine
   graph), exe.migration (swap+migrate), lng.round_trip (round-trip).
5. **Execution semantics** (chs. 4, 15): disciplines, region inference,
   the seven mappings, per-sample islands, quiescence. Gate: exe.region_inference, exe.realtime_safety, exe.canonical_mappings, exe.per_sample_islands, exe.quiescence_and_demand,
   tcf.mapping_guarantees, tcf.swap_safety, golden-audio hello-cosine with live edits.
6. **The store** (ch. 3): object machinery + store-graph face + commit
   paths + refs + op-tree history. Gate: sto.object_machinery, sto.store_graph_face, sto.commit_paths, sto.refs_and_undo, sto.provide_compatible, sto.fetch, sto.composite_graphs, sto.retention_and_forgetting, sto.back_link_index, ADR-018 undo tests.
7. **Compile and realize** (ch. 5): engine v0 as data, deterministic compile
   + map, projection editing, interpret backend. Gate: cmp.compile_is_derivation, cmp.determinism_and_map, cmp.extension_ports, cmp.projection_editing, cmp.fork, cmp.laziness, cmp.identity_across_recompilation.
8. **Packages, in probe-proven order** (ch. 7): audio first (kernels
   salvaged from synth_core — they survive verbatim), then render, worker,
   then the codegen backend (freezer: frz.round_trip, frz.tier_computation, frz.freestanding_proof — movements now
   machine-frozen, closing the loop with rung 2), then XR.
9. **The mesh** (ch. 8, 14): keys/pairing, advertisement, fetch,
   ops-to-arbiter, placement. Gate: msh.keypairs_pairing, msh.authenticated_transport, msh.three_lists, msh.pull_shaped_placement, msh.graphs_vs_plugins, msh.capture_testimony_keys, msh.discovery, msh.graded_circles, fmt.wire_transcripts transcripts, pkg.net_package, pkg.placement_as_fallthrough;
   peer-level conformance is born here.
10. **Hosts** (ADR-019): CPython module, browser/wasm. Gate: abi.three_packagings,
    python-executes-graph notebook test.
11. **Surfaces**: editor patches, store browser, documents. Gate: edr.editor_is_nodes, edr.defaults_discipline, edr.undo, edr.realized_view_editing, edr.store_browser, edr.documents, edr.agents_as_peers, edr.observability.
12. **The self-hosting closure** (ch. 17): the suite as datasets, the
    candidate-as-peer harness, sygaldreye-N derives N+1. Gate: cnf.suite_as_data, cnf.candidate_as_peer, cnf.two_profiles, cnf.succession_end_to_end, cnf.self_gate —
    at which point the book, the suite, and the system are one object and
    the probe can be archived with honors.

**Salvage list from the probe**: synth_core kernels (verbatim — aut.kernel_contract
already enforced), the ugen/lift/editor test suites (become golden corpus
seeds and blessing candidates), editor.json + card.json (the editor's
decomposed form re-expressed in new topology format), the HRTF/audio device
machinery (audio package machinery), scene/demo graphs (conformance
corpus). Everything else is reference reading.
