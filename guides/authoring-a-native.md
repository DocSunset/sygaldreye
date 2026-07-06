# Authoring a native

A *native* is a node whose interior is C++ instead of a graph. This guide is
mostly about **not writing one** — and, when you must, writing the smallest
honest one.

## First: are you sure?

L22, a guiding star equal in rank to no-restarts: **make graphs, not C++.**
Every new behavior defaults to a graph. A native is a standing exception that
has to earn its place, because a graph line is live-editable, provenance-
tracked, placeable, and freezable, while a native line is opaque and toolchain-
bound. Before you declare a native, prove you can't compose it from existing
nodes. If you *could* compose it but it's too slow, the answer is still not a
native: **when a graph is too slow, freeze it** (see `authoring-a-graph.md` →
Freezing) — the freezer emits the same math as native code with none of the
opacity.

A native is justified only by a **declared clause** (ADR-033, ADR-036). Pick
exactly one and write it on line 1 of the source file — the gate (COR-5.1)
rejects any native without one:

| clause | when |
|---|---|
| `machinery` | it touches the world — a device, the OS, a thread, a syscall, a trap, an interpreter. Its interior genuinely couldn't be links. |
| `floor` | it's a per-sample kernel (AUT-1): the arithmetic the graph substrate bottoms out in. |
| `maturity` | it wraps an already-tested implementation — you're importing its test suite, not its line count. |
| `fixture` | it exists only to exercise or demonstrate a contract from the conformance side (an audit probe, a fault provocateur, an ABI demo). Permanent, never shipped in a movement. |
| `scaffolding` | a temporary walk that will dissolve — it **must** name the criterion that retires it (`// clause: scaffolding (dissolves: CMP-9.2)`), and the dissolution gate fails the suite the moment that criterion goes green. |

If none of the first four fit and it isn't scaffolding, you probably want a
graph.

## The shape of a native

A native is two things: a **port declaration** (data) and a **`native_type`**
(the C++ behaviour). The declaration is the source of truth — ports, promises,
the canonical codec, and the language bindings all generate from it (AUT-3, one
declaration → everything). You never hand-write a codec or a serializer; if you
find yourself doing that, you've already failed FMT-1.

### 1. Declare the ports

Add a struct to `src/nodes/hello_natives/endpoints.hpp` (or a package's
equivalent). Ports are typed by **(kind, discipline)**:

```cpp
namespace au = syg::authoring;

struct osc {
  au::in<au::scalar, au::value>  freq;   // a value-discipline scalar input
  au::in<au::scalar, au::value>  shape;
  au::out<au::audio, au::block>  out;    // a block-clocked audio output
};
```

Kinds (`vocabulary/kinds.json` is the catalog, read as data — no hardcoded
switch anywhere): `scalar`, `audio`, `bang`, `span`, `graph`, `text`, `ops`,
`cidset`, `fault`, … Disciplines (ADR-020, three forever): `event` (push),
`value` (dirty-push / demand-pull, cadence-free), and a clock name — `block`
(the audio clock) or `frame` (the control clock). The **(kind, discipline)**
pair is all the region inferencer and the connection oracle need; legality of
any edge is decided by that one first-order oracle, never by a special case.

The generator turns this struct into `osc_in_ports()` / `osc_out_ports()`
(descriptors), the dag-cbor codec, the Python/TS bindings, and the JSON
projection. That's the whole point of declaring first.

### 2. Write the behaviour

A `native_type` (see `src/crown/crown.hpp`) is a table of function pointers.
You fill in only the ones your disciplines use:

```cpp
struct knob { float k = 0.0f; };
void knob_set(void* s, const char* port, double v) {
  if (!std::strcmp(port, "k")) static_cast<knob*>(s)->k = float(v);
}
void scale_value_tick(void* s, double /*dt*/, const float* ins, float* outs) {
  outs[0] = ins[0] * static_cast<knob*>(s)->k;   // out = in * k
}

extern const syg::crown::native_type scale_native;
const syg::crown::native_type scale_native{
    "scale",
    [] { return static_cast<void*>(new knob()); },   // create (L9: no resource
    [](void* s) { delete static_cast<knob*>(s); },   //   acquisition here)
    knob_set, /*set_text=*/nullptr,
    /*process=*/nullptr, scale_value_tick,           // pick the discipline hook
    syg::generated::scale_in_ports(),                // <- generated, never hand-written
    syg::generated::scale_out_ports()};
```

Which hook you fill in depends on the discipline of your ports:

- **`process(void*, const float* const* ins, float* const* outs, int frames)`**
  — the block/audio path. This is where kernels live. It must be freestanding-
  clean (COR-1): no allocation, no absolute time, no blocking. See the AUT-1
  kernel contract below.
- **`value_tick(void*, double dt, const float* ins, float* outs)`** — the
  value/frame path: recompute outputs from inputs after `dt`. Cadence-free.
- **`apply(void*, const char* port, double v)`** — the event applier: consume an
  event, write state. Runs *before* process at the consumer's boundary, never
  mid-tick.
- **`svalue_tick` / `sapply` / `semit`** — the structured lane (ADR-034,
  LNG-11): value recompute, event applier, and pull-emit over **kind-tagged
  payloads** (graph, ops, text, …). This is how a node emits an edit-op event
  (`op_button`) or consumes a graph. Structured payloads ride event and value
  only — the oracle refuses them on stream, which is why the RT audits and
  golden audio can never move.

Flags worth knowing: `clocked` (wired to its executor's clock), `block_override`
(an FFT-shaped whole-block interior — a cycle through it demands an explicit
delay at edit time), `resource_holder` (owns a device; refuses to be lifted).

### 3. The kernel floor (AUT-1)

If your clause is `floor`, the process body is a kernel and must obey the
salvage contract that let the probe's `synth_core` survive verbatim:

- **per-sample, `dt`-based, no absolute time** — a kernel never reads a clock;
  it advances by the `dt` it's handed, so N=1 and N=128 produce identical
  output.
- **bounded outputs**, stage machines for envelopes, delay-at-N honored, slew
  clamps respected. `syg kernel-audit` exercises exactly these.
- no allocation on the audio path, ever (ABI-2 audits 10k callbacks for zero
  RT allocs).

### 4. Register it

Natives are present by **linkage** (SZ-2): add the name to the generator's
native list in `src/generator/generate.cpp` (`all_natives_list`) and, for a
declared struct, an `emit<syg::nodes::decl::osc>(o, "osc", …)` line. The
generated registration TU references each native's symbol, so a package that
omits it (via `SYG_OMIT`) simply drops the object — `nm` proves the absence.
Once registered, your native appears in the palette (`registry_face`)
indistinguishable from a subgraph, a frozen artifact, or a loaded plugin — four
authoring routes, one registry.

## The loop

Same as everything else in this repo (see `BUILDER.md`): **write the test
first** from the criterion, then the code; run `conformance/gates.sh` and
`python3 conformance/run.py`; build inside the dev shell:

```
nix develop -c ninja -C build syg
```

Commit with the criterion id in the message. Line 1 carries the clause. If the
compiler warns, treat it as an error.

## The rule of thumb

The native ledger (ch. 16) expects ~70 natives at maturity, half of them
generated one-line kernels. If your new native isn't a kernel, a piece of
world-touching machinery, a matured import, or a fixture — stop, and go author
a graph.
