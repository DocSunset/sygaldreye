# Endpoints v6: one struct, directional shapes, no values map
# (ratified in conversation, Travis + Claude 2026-06-12; subsumes
# inlet_defaults.md rungs 2-4 — rung 1, subgraph params, ships first
# and independently)

## The collapse
One `endpoints` struct per node (inputs/outputs structs merge).
Direction is the wrapper trait, not the enclosing struct. Names come
from boost::pfr::core_name (already a dep) — fixed_string name params
die. C++26 reflection later thins the wrappers further; it cannot
replace them (ranges, units, bang-vs-toggle, persisted fields live in
the wrapper).

## The six shapes
- in<T, Def>           connection-only; compile-time default; total
                       get() = src ? *src : Def. No null branches in
                       consumers EVER. Arithmetic defaults via fp()
                       NTTPs; non-structural payloads (Eigen, AudioBuffer)
                       via constexpr provider/trait — NB Eigen default
                       ctor is UNINITIALIZED, traits must say Zero().
- normalled_in<T, ...> runtime default = persisted param + widget
                       (patchbay normalling semantics). Optional
                       range<min,max> / unit<"..."> metadata tags.
- cv_in<T, S>          {src, offset, slope}; get() = src ?
                       cv_apply(*src, slope, offset) : offset.
                       cv_apply customization point: affine for
                       arithmetic/vector, slerp-compose for quat
                       (slope = amount, offset = base), =delete where
                       meaningless. offset+slope are persisted params
                       (attenuverter knobs).
- event_in / event_out events are deliveries, not values; queue
                       machinery owns region crossings (unchanged).
- out<T>               owned value storage; optional range metadata
                       (meters).

## Derived-qualities table (one table drives everything)
payload type → admissible shapes, widget, persistence, cv_apply:
  value semantics (scalar/bool/text/vec2/3/4) → all shapes
  quat → all shapes, slerp cv_apply
  stream/GPU (audio, texture, draw_call, mesh) → in<T> only;
    null = absent/silence is their natural unconnected state
No required<T>: connectivity is a runtime graph property; nodes behave
sensibly unwired (patcher tradition). Wire-time schema warning at most.

## Purity rule (ratified)
An endpoint may contain only stateless, per-sample, parameter-shaped
behavior — a pure function of (source, params). Anything with memory or
time (slew, smoothing, hysteresis) is a NODE: state wants migration,
observability, and a card. Litmus: could two consumers share it, or
does it remember anything? Either yes → node.

## True edges become literal
Wiring = repointing in.src at the producer's out<T> member, once per
graph swap (post-migration, where references already re-resolve).
EdgeApplier dissolves. Per-tick edge copies: zero. Fan-in becomes
structurally impossible (one pointer per inlet; summing is a mix node —
the ratified rule, now enforced by types). Cross-region consumers point
at the MAPPING's slot (latch/ring output), never into another region's
node — "mappings are just nodes with throughpoints", literally.

## The values map dies
Execution no longer needs it. Observability becomes PULL: /values (and
the bridge's hosted-node mirroring) queue a request fulfilled at the
next frame boundary (screenshot cv pattern); push_outputs becomes the
on-demand probe hook into a per-request store. Zero cost unobserved.
Persistent/wired observation = probe nodes on edges (the design table's
probe mapping). No ambient global.

## ABI v6
- descriptors: connect(port_name, const void* src) added; set_*_in
  family becomes the DEFAULT/param writers (used by /param, latches,
  bridge, subgraph inlets — shapes unchanged).
- make_descriptor rewritten around one struct + shape traits.
- serialize emits persisted endpoint fields (normalled defaults,
  cv offset/slope) — never live/edge values (the inlet_defaults fix,
  inherited here).
- Old helpers (slider/toggle/text/port/bang) become deprecated aliases
  onto the new shapes for mechanical per-file migration; aliases deleted
  at the end.

## Order & interlock
endpoints v6 → text edges (PortValue string + event text) → speech
nodes → kernel extraction → freezer. Everything downstream gets cheaper
after the collapse; the freezer's portable POD components and live nodes
converge on the same endpoint shapes.

## Tests that gate it
- zero-copy: tick a chain, assert producer buffer address == what the
  consumer read (no copies).
- normalled fallback: disconnect mid-run → consumer reads persisted
  default after swap.
- cv_apply quat: slope 0 → offset; slope 1 → offset * src.
- serialize-mid-modulation captures defaults (regression from
  inlet_defaults).
- pull-observability: /values absent until requested; request returns
  frame-coherent snapshot; bridge mirroring unaffected.

## Progress (2026-06-13 overnight)

- Phase A LANDED (74f166e): shapes (in/normalled_in/cv_in/out/event_*),
  endpoint_default trait, cv_apply (affine + quat slerp), ABI v6
  connect/output_ptr, persisted-only serialize with <name>_slope keys.
  Six gate tests green on host; android cross-build clean.
- Phase B LANDED (f88a49d): plan->wires for v6↔v6 same-region edges,
  wire_plan (reset-then-connect, post-migration stale-pointer safe),
  executor gate test (pointer equality + default fallback after swap).
- Phase C (node migration) DELIBERATELY DEFERRED: ugens get rewritten
  by kernel extraction anyway — they'll be born v6-native there rather
  than migrated twice. Remaining clusters (math_nodes, xr_sources,
  scene/render nodes) migrate after kernels. Transitional semantics for
  mixed edges: legacy→v6 normalled works via the set_* param writers
  (with the known serialize-mid-modulation wrinkle, transitional only);
  legacy→v6 in<T> streams are DEAD edges — migrate stream producers and
  consumers in the same cluster.
- Values-map death (phase D) gated on full migration.

## Progress (2026-06-12, the unification day)
- Phase C COMPLETE: all 97 registered node types carry one endpoints
  struct. Clusters: math (24), xr sources, trigger/ptt, ui, net, hand,
  fly_camera, spawner, curtain, cube, text_label, mic/wav/sample/
  spectrogram, poke pair, slew, speech+agents (stt/tts/whisper/claude/
  editor), GPU scene cluster, mesh family, render plumbing.
- DELETED instead of migrated (ratified rule): seven baked synths +
  audio_scene/MonoSynth, aurora monolith (aurora.json supersedes),
  float_mapper (unregistered duplicate of scale).
- Subgraphs forward wires: SubgraphDescriptor connect/output_ptr reach
  inner storage; inner plan built eagerly (lazy build would reset
  outer-forwarded srcs).
- Legacy machinery DELETED: slider/toggle/bang/text/port shapes, the
  two-struct make_descriptor branches, PortField concepts, legacy
  to_json/from_json. make_descriptor requires HasEndpoints.
- Compat slots narrowed to their real meaning: region-crossing stream
  edges into v6 consumers (ring → plan slot → src).
- Name-collision conventions: ::in/::out qualification keeps in/out
  field names; processors read mesh/texture, write mesh_out/
  texture_out; hand renamed param inputs (trigger_in...), fly_camera
  renamed passthrough outputs (yaw_out...) — HTTP contracts unchanged.
- Phase D (values-map death) now unblocked.

## Phase D LANDED (2026-06-12 evening, 01ce20e)
Graph::values DELETED. Execution reads producer storage (read_output =
output_ptr + kind; events expose their flag). Crossings deliver
themselves (ring→slot, snapshot/queue→consumer, latch←storage). Pull
observability: snapshot_values sweep on demand; /values + probe block
until the next frame boundary; bridge hosted nodes are standing
observers; render thread reads storage directly (camera.pv). Block
store died too — the audio callback makes zero map writes. Device
verified: ding chain live, 86-key frame-coherent snapshots on demand.
ALL FOUR PHASES COMPLETE — this item is done.
