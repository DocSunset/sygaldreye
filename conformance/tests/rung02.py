"""Rung 2 — stubs. Each entry: criterion id -> callable or None (pending).
Write the test when you reach this rung; the assertion is quoted from the book.
Never weaken a test to pass it — amend the book by ADR instead."""

TESTS = {
    # 13-native-contract.md: adding a field to a struct surfaces port + codec + binding with
    "ABI-1.1": None,
    # 13-native-contract.md: grep gate — no hand-written `try/catch` or serializer in node
    "ABI-1.2": None,
    # 13-native-contract.md: EXE-3.1's instrumented run extended over the full hook table.
    "ABI-2.1": None,
    # 13-native-contract.md: a node acquiring a device in create() fails a debug assert with
    "ABI-2.2": None,
    # 13-native-contract.md: a declared parser node converts a malformed-input exception to a
    "ABI-3.1": None,
    # 13-native-contract.md: an undeclared throw in a quarantined plugin kills the subprocess;
    "ABI-3.2": None,
    # 16-the-core.md: The escapement compiles freestanding;
    "COR-1": None,
    # 16-the-core.md: CI gate: the core-named list in this
    "COR-3": None,
    # 06-stage0-boot.md: CI grep gate. SZ-1.2: same object code (per-arch) links into
    "SZ-1.1": None,
    # 06-stage0-boot.md: at most 10 lines each; CI line-count gate.
    "SZ-6": None,
}
