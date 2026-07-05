# oracle — the promise oracle

Owning package: `naming`. Allowed dependencies: none (std only).
Spec: ch. 2 NAM-5, ADR-020/030 — one plain first-order program over
(kind, discipline) pairs, consulted at edit/commit time by every surface
(parse-time validation, editor wire-drop), never at tick time (NAM-5.4).

Discipline vocabulary: `event`, `value` (cadence-free), or a clock name
(`block`, `frame`, …) for clocked stream ports — clocks are open (ADR-020).
Verdicts: kinds must agree; same discipline is a true edge; a crossing into
a clocked port is bridged by `latch`, into `value` by `snapshot` (the ch. 1
LFO→gain latch). The table grows under TCF-1 (rung 5); it only ever names
one of the seven core mapping definitions.
