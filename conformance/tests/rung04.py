"""Rung 4 — stubs. Each entry: criterion id -> callable or None (pending).
Write the test when you reach this rung; the assertion is quoted from the book.
Never weaken a test to pass it — amend the book by ADR instead."""

TESTS = {
    # 04-execution.md: swap hello-cosine for a version with an added noise0: osc0's phase
    "EXE-5.1": None,
    # 04-execution.md: reordering editor cards (lift_key = id) preserves each card's drag
    "EXE-5.2": None,
    # 11-language-core.md: property test over the graphs_dir corpus: serialize(parse(j))
    "LNG-8.1": None,
    # 06-stage0-boot.md: deleting a native's object breaks the link naming the symbol.
    "SZ-2.1": None,
    # 06-stage0-boot.md: with the store package broken, the resolver still loads a graph
    "SZ-3.1": None,
    # 06-stage0-boot.md: `unfreeze(stage0)` recovers tape + manifest; rebuild compares.
    "SZ-4.1": None,
    # 06-stage0-boot.md: kill stage 1 a hundred times; restart each time; SZ-5.2: edits addressed at
    "SZ-5.1": None,
    # 06-stage0-boot.md: Empty object directory to live engine graph
    "SZ-7": None,
    # 06-stage0-boot.md: Escapement + crown + tape reaches full
    "SZ-8": None,
}
