# executor — execution semantics (the rung-5 package)

Owning package: `executor`. Allowed dependencies: `crown`, `organs`
(parser, registry-face, promise oracle via `naming`), nlohmann_json.
Spec: ch. 4 + ch. 15, ADR-013/015/020. Components grow here through the
rung: `regions` (inference — never declared), the segment plan, the seven
mappings, pacing (pump_offline first).

Regions: recomputed on every edit; block membership = sink closure through
audio edges upstream; a node with audio in but no audio out stays
frame-side; everything else frame (worker/net when their packages exist).
Boundary mappings come from the ONE promise oracle (NAM-5) reading the
generated port promises — the same first-order program every surface uses.
