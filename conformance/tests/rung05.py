"""Rung 5 — stubs. Each entry: criterion id -> callable or None (pending).
Write the test when you reach this rung; the assertion is quoted from the book.
Never weaken a test to pass it — amend the book by ADR instead."""

TESTS = {
    # 04-execution.md: hello-cosine's plan holds raw pointers for edges 0 and 2 (true
    "EXE-1.1": None,
    # 04-execution.md: saving mid-modulation persists gain's *default*, not the LFO's
    "EXE-1.2": None,
    # 04-execution.md: a one-node Karplus-Strong loop (delay-line kernel + damping,
    "EXE-10.1": None,
    # 04-execution.md: inserting an explicit block-sized delay into the loop reverts
    "EXE-10.2": None,
    # 04-execution.md: wiring a spectrogram (block-override) into a cycle yields an
    "EXE-10.3": None,
    # 04-execution.md: frozen versus interpreted render of the same feedback patch:
    "EXE-10.4": None,
    # 04-execution.md: a static scene (no clock-wired nodes, no incoming events)
    "EXE-11.1": None,
    # 04-execution.md: one param write recomputes exactly the dirty downstream cone,
    "EXE-11.2": None,
    # 04-execution.md: the inert-node lint flags a node with no demanded output and no
    "EXE-11.3": None,
    # 04-execution.md: event delivery is unaffected by quiescence: a bang into a
    "EXE-11.4": None,
    # 04-execution.md: hello-cosine infers {osc0, vca0, dac0} block, {lfo0} frame.
    "EXE-2.1": None,
    # 04-execution.md: deleting edge 2 (vca0 to dac0) moves osc0/vca0 out of the block
    "EXE-2.2": None,
    # 04-execution.md: instrumented allocator + lock detector report zero events across
    "EXE-3.1": None,
    # 04-execution.md: step lfo0 to a constant 0.7 to vca0 sees exactly 0.7 from the next
    "EXE-4.1": None,
    # 04-execution.md: N palette-press events across threads arrive N times, in order.
    "EXE-4.2": None,
    # 04-execution.md: a feedback cycle certifies tick-1 delay semantics (existing
    "EXE-4.3": None,
    # 04-execution.md: grep-level check: no `::instance()` reach from node code into
    "EXE-6.1": None,
    # 04-execution.md: with no audio device, pump_offline paces the block region and
    "EXE-6.2": None,
    # 04-execution.md: \"render 2 s of hello-cosine offline\" produces a wav dataset with
    "EXE-7.1": None,
    # 04-execution.md: the latch in hello-cosine appears as a node card; replacing it
    "EXE-8.1": None,
    # 04-execution.md: removing lfo0 and edge 1 in one edit op collects lfo0's state;
    "EXE-9.1": None,
    # 11-language-core.md: enumerating the kind registry yields the catalog above; each
    "LNG-1.1": None,
    # 11-language-core.md: N-by-3 span to vec3 input stamps N clones; N=1 degenerates to one;
    "LNG-2.1": None,
    # 11-language-core.md: N-by-3 span to color_mesh instance input draws N instances in ONE
    "LNG-2.2": None,
    # 11-language-core.md: 10,000 bangs through [button to queue to counter across threads]
    "LNG-3.1": None,
    # 11-language-core.md: EXE-1.2 / EDR-2.1 (defaults never capture live modulation).
    "LNG-4.1": None,
    # 11-language-core.md: the widget table is data-driven: adding range metadata to a
    "LNG-4.2": None,
    # 11-language-core.md: recording the ops of an editor session and replaying them onto
    "LNG-5.1": None,
    # 11-language-core.md: an op arriving over the mesh carries its author peer key
    "LNG-5.2": None,
    # 11-language-core.md: `card.json` in graphs_dir appears in the palette; spawning it
    "LNG-6.1": None,
    # 11-language-core.md: a subgraph containing dac is refused lifting with the
    "LNG-6.2": None,
    # 11-language-core.md: static audit (EXE-6.1's grep) plus: a subgraph two levels deep
    "LNG-7.1": None,
    # 11-language-core.md: Design the text event payload so graph edits
    "LNG-9": None,
    # 15-time-concurrency-faults.md: Each row of the table has a stress test:
    "TCF-1": None,
    # 15-time-concurrency-faults.md: 10,000 random swaps under load: no torn values, no
    "TCF-2": None,
    # 15-time-concurrency-faults.md: No wall-clock syscalls from any node
    "TCF-3": None,
    # 15-time-concurrency-faults.md: One test per fault class by region kind:
    "TCF-4": None,
    # 15-time-concurrency-faults.md: A frozen movement build contains no fault
    "TCF-5": None,
}
