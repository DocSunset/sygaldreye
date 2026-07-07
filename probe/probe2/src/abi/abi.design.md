# abi — the native-contract runtime

Owning package: `abi`. Allowed dependencies: none (std only).
Spec: ch. 13 — the hook table's threading and discipline promises, made
observable: a thread-local phase, per-phase allocation/lock counters (the
EXE-3.1 instrumentation, extended over the full hook table — ABI-2.1), the
resource-acquisition check (`acquire_device` asserts prepare — ABI-2.2,
L9), and the fault record shape shells emit (ABI-3).

- Inputs: phase transitions from generated shells; alloc events from the
  harness allocator override (`audit_alloc.cpp`, linked into debug/harness
  binaries only).
- Outputs: counters per phase; a loud abort with the allocation-discipline
  message on a create()-time acquisition.
- Intended seams: rung 5's executor regions reuse the same guards; the
  quarantine tier (ABI-3.2) wraps whole plans in a subprocess.
