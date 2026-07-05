"""Rung 1 — stubs. Each entry: criterion id -> callable or None (pending).
Write the test when you reach this rung; the assertion is quoted from the book.
Never weaken a test to pass it — amend the book by ADR instead."""

TESTS = {
    # 14-formats-protocols.md: Encoder conformance: a corpus of value/bytes pairs (grown per
    "FMT-1": None,
    # 14-formats-protocols.md: Address grammar: parse-after-print identity property test (NAM-1.1)
    "FMT-2": None,
    # 14-formats-protocols.md: Every pin above is FROZEN as of 2026-07-05; changing one is a
    "FMT-5": None,
    # 02-data-naming-kinds.md: `parse(print(a)) == a` for all addresses (property test).
    "NAM-1.1": None,
    # 02-data-naming-kinds.md: `#a11/nodes/osc0/out` resolves identically on any peer holding
    "NAM-1.2": None,
    # 02-data-naming-kinds.md: `graphs/hello-cosine:nodes/osc0/freq` is live; after
    "NAM-2.1": None,
    # 02-data-naming-kinds.md: moving ref `graphs/hello-cosine` re-delivers to all live-address
    "NAM-2.2": None,
    # 02-data-naming-kinds.md: inserting `nodes/noise0` into #b22's topology does not change the
    "NAM-3.1": None,
    # 02-data-naming-kinds.md: a `scalar` on a wire and a committed `scalar` dataset carry the
    "NAM-4.1": None,
    # 02-data-naming-kinds.md: `osc0/out (audio, block) to vca0/in (audio, block)` legal, true edge.
    "NAM-5.1": None,
    # 02-data-naming-kinds.md: `lfo0/out (scalar, frame) to vca0/gain (scalar, block)` legal via
    "NAM-5.2": None,
    # 02-data-naming-kinds.md: `draw_call to audio` is ILLEGAL (both surfaces reject identically).
    "NAM-5.3": None,
    # 02-data-naming-kinds.md: no kind/promise lookup occurs during tick (assert via instrumentation
    "NAM-5.4": None,
    # 02-data-naming-kinds.md: re-hashing fetched data verifies it; a corrupted byte is detected.
    "NAM-6.1": None,
    # 02-data-naming-kinds.md: two takes sharing a chunk store it once (dedup observable in
    "NAM-6.2": None,
    # 02-data-naming-kinds.md: after inserting a line at the top of a text dataset, a link
    "NAM-7.1": None,
}
