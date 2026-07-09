# Chapter 13 — The native contract (ABI)

*The boundary between graph and machinery. A builder implementing this chapter
delivers: the hook set and its threading promises, the generated shells, the
plugin packagings, and contract succession. Governing ADRs: 016, 017, 019,
025, 028.*

## Design

**One declaration.** A node type is authored as one endpoints struct (real
named fields; PFR now, static reflection later). Everything else is
generated from it (aut.generated_descriptors, ADR-017): the descriptor (ports, their promises, affordance
metadata), the codecs, the bindings, and the shell that adapts the author's
code to the hooks below. Authors write a struct and behavior; they never
write serialization, registration, or try/catch.

**The hooks.** All hooks are `noexcept` at the C surface; exceptions never
cross the boundary.

| hook | called by | may | must not |
|---|---|---|---|
| `create(state)` | the crown/applier, at a tick boundary | allocate state members | acquire resources, block, fail |
| `prepare(rates, maxima)` | executor, before first tick in a region | size buffers, acquire the region's resources lazily | be called on a tick path twice |
| `apply(event)` | region thread, before process, topological order | consume events, write state | emit into upstream |
| `process(inputs, state)` | region thread at region cadence (or per-sample within an island) | read resolved inputs, write outputs | allocate/lock/syscall in RT regions, block outside worker |
| `destroy(state)` | the crown/applier, after unlinking | release what prepare acquired | touch other instances |
| `set_context(ctx)` | pump, resource holders only | receive graph/queue/registry seams | be a global reach |

Threading promise: a node's hooks are called from exactly one region thread
at a time; cross-region data arrives only through mappings; the swap boundary
is the only moment instances move between plans (state carried by route).

**Fallibility is declared (ADR-016).** Default: noexcept-by-contract, no
wrapper anywhere. A node that declares a fault output gets a generated catch
in its shell converting exceptions into fault-record values on that output.
An undeclared throw is `std::terminate` — containment-unit death, testified.

**The contract is a kind, so versioning is succession (ADR-025).** There is
no ABI version integer: the contract itself is a spec node; a changed
contract is a successor with a declared migration where mechanical. A plugin
records which contract hash its shell was generated against; loadability =
reachability. (The probe's v1–v8 integer history ends here.)

**Packagings.** The same generated shell wraps: statically linked natives
(registration TU — sz.generated_registration), `.so` plugins (dlopen, hot-reload), and WASM side
modules (browser/Node). All are datasets with recipe provenance; all pass the
msh.graphs_vs_plugins gate; fresh plugins realize in the quarantine tier (subprocess) until
trust promotes them.

## Requirements

**abi.one_declaration** Descriptor, codecs, shell, bindings generated
from the endpoints struct; zero hand-written adapters in the tree.
- abi.one_declaration.field_surfaces_port: adding a field to a struct surfaces port + codec + binding with
  no other edits (aut.generated_descriptors.field_surfaces_descriptor restated at the ABI level).
- abi.one_declaration.no_handwritten_adapters: grep gate — no hand-written `try/catch` or serializer in node
  sources.

**abi.hook_discipline** RT hooks allocation/lock/syscall-free after
prepare; create never acquires resources.
- abi.hook_discipline.rt_hooks_clean: exe.realtime_safety.zero_alloc_lock's instrumented run extended over the full hook table.
- abi.hook_discipline.create_acquires_fails: a node acquiring a device in create() fails a debug assert with
  the allocation-discipline message.

**abi.fault_declaration** Only declared-fallible shells contain a
catch; undeclared throws kill the containment unit and testify.
- abi.fault_declaration.declared_fault_recorded: a declared parser node converts a malformed-input exception to a
  fault record on its output; the region keeps ticking.
- abi.fault_declaration.undeclared_throw_kills: an undeclared throw in a quarantined plugin kills the subprocess;
  testimony names the node route; the supervisor's policy patch restarts per
  its wiring.

**abi.contract_succession** Plugins record their contract hash;
loading checks reachability, not integer equality.
- abi.contract_succession.loads_iff_reachable: a plugin generated against contract C1 loads on a peer speaking
  C2 if and only if C2 declares a migration path from C1; refusal is typed and names
  the missing path.

**abi.three_packagings** The same node source builds as
linked native, .so, and WASM side module and passes the same behavioral
tests (aut.four_routes.four_routes_interchangeable extended to the WASM form).
