"""Rung 7 — stubs. Each entry: criterion id -> callable or None (pending).
Write the test when you reach this rung; the assertion is quoted from the book.
Never weaken a test to pass it — amend the book by ADR instead."""

TESTS = {
    # 05-compilation.md: compiling hello-cosine twice runs passes once (counter).
    "CMP-1.1": None,
    # 05-compilation.md: editing the *defaults* node only does not re-run structural passes
    "CMP-1.2": None,
    # 05-compilation.md: compile hello-cosine a hundred times; one distinct output hash.
    "CMP-2.1": None,
    # 05-compilation.md: the map contains `nodes/osc0 to block/osc0` (or equivalent stable
    "CMP-2.2": None,
    # 05-compilation.md: with the audio package spliced, the engine graph diff against
    "CMP-3.1": None,
    # 05-compilation.md: pass execution order equals topological order of the engine patch
    "CMP-3.2": None,
    # 05-compilation.md: replace hello-cosine's latch with `smoother` in the execution
    "CMP-4.1": None,
    # 05-compilation.md: an edit whose target route vanished upstream surfaces a conflict
    "CMP-4.2": None,
    # 05-compilation.md: fork hello-cosine's execution graph, edit upstream app graph,
    "CMP-5.1": None,
    # 05-compilation.md: steady-state hello-cosine playback instantiates zero engine
    "CMP-6.1": None,
    # 05-compilation.md: while sounding, add noise0 to hello-cosine, re-compile, swap:
    "CMP-7.1": None,
}
