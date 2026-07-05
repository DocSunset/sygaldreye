"""Rung 11 — stubs. Each entry: criterion id -> callable or None (pending).
Write the test when you reach this rung; the assertion is quoted from the book.
Never weaken a test to pass it — amend the book by ADR instead."""

TESTS = {
    # 09-editor-documents.md: replacing the palette subgraph live (swap+migrate) changes editor
    "EDR-1.1": None,
    # 09-editor-documents.md: modulate vca0/gain with lfo0, save, reload: gain default is the
    "EDR-2.1": None,
    # 09-editor-documents.md: sequence [add node, drag param, remove node, undo, undo] restores
    "EDR-3.1": None,
    # 09-editor-documents.md: scripted gesture test drives the smoother replacement of CMP-4.1
    "EDR-4.1": None,
    # 09-editor-documents.md: walk ground to graphs/hello-cosine to topology to osc0 to type osc  to
    "EDR-5.1": None,
    # 09-editor-documents.md: frontier of a node with 100,000 links paginates without blocking the
    "EDR-5.2": None,
    # 09-editor-documents.md: a document transcluding `graphs/hello-cosine:nodes/osc0/freq`
    "EDR-6.1": None,
    # 09-editor-documents.md: round-trip metric harness: decompose a small permissive C++ file
    "EDR-6.2": None,
    # 09-editor-documents.md: the full editor integration suite runs twice — human-input
    "EDR-7.1": None,
    # 09-editor-documents.md: a probe mapping attached to any edge exposes its value stream
    "EDR-8.1": None,
}
