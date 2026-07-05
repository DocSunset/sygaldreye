# Chapter 13 — The native contract (ABI)

*The boundary between graph and machinery. A builder implementing this chapter
delivers: the hook set and its threading promises, the generated shells, the
plugin packagings, and contract succession. Governing ADRs: 016, 017, 019,
025, 028.*

## Design

**One declaration.** A node type is authored as one endpoints struct (real
named fields; PFR now, static reflection later). Everything else is
generated from it (AUT-3, ADR-017): the descriptor (ports, types, affordance
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
(registration TU — SZ-2), `.so` plugins (dlopen, hot-reload), and WASM side
modules (browser/Node). All are datasets with recipe provenance; all pass the
MSH-5 gate; fresh plugins realize in the quarantine tier (subprocess) until
trust promotes them.

## Requirements

**ABI-1 (one declaration).** Descriptor, codecs, shell, bindings generated
from the endpoints struct; zero hand-written adapters in the tree.
- ABI-1.1: adding a field to a struct surfaces port + codec + binding with
  no other edits (AUT-3.1 restated at the ABI level).
- ABI-1.2: grep gate — no hand-written `try/catch` or serializer in node
  sources.

**ABI-2 (hook discipline).** RT hooks allocation/lock/syscall-free after
prepare; create never acquires resources.
- ABI-2.1: EXE-3.1's instrumented run extended over the full hook table.
- ABI-2.2: a node acquiring a device in create() fails a debug assert with
  the allocation-discipline message.

**ABI-3 (fault declaration).** Only declared-fallible shells contain a
catch; undeclared throws kill the containment unit and testify.
- ABI-3.1: a declared parser node converts a malformed-input exception to a
  fault record on its output; the region keeps ticking.
- ABI-3.2: an undeclared throw in a quarantined plugin kills the subprocess;
  testimony names the node route; the supervisor's policy patch restarts per
  its wiring.

**ABI-4 (contract succession).** Plugins record their contract hash;
loading checks reachability, not integer equality.
- ABI-4.1: a plugin generated against contract C1 loads on a peer speaking
  C2 iff C2 declares a migration path from C1; refusal is typed and names
  the missing path.

**ABI-5 (three packagings, one shell).** The same node source builds as
linked native, .so, and WASM side module and passes the same behavioral
tests (AUT-5.1 extended to the WASM form).
