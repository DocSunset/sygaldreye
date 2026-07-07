# supervisor — spawn-and-park machinery

Owning package: `organs`. Clause: **machinery** (a thread/process watcher —
its interior couldn't be links). Allowed dependencies: `crown`, POSIX.
Spec: SZ-5 / ADR-016 — the frozen supervision base case: spawn stage 1,
park, restart per the wired policy (restart-limit param); stage 0's own
graph rejects runtime edits with a clear error (SZ-5.2). Richer policy
(intensity windows, backoff, alerting) is WIRING, not code here.
