# Chapter 8 — Mesh and trust (MSH)

*A builder implementing this chapter delivers: peer identity (keypairs +
pairing), authenticated transport, the three sovereign lists, the
graphs-vs-plugins gate, and the scale posture.*

## Design

**Identity.** Each peer holds a keypair; joining is a one-time pairing
ceremony; the mesh is "the set of keys my peers accept." Flat one-domain
topology for now — nobody special, everything visible inside — with device
revocation, unforgeable capture testimony (testimony's peer id IS a public
key), and signable provenance. Transport authentication/encryption falls out;
the historical unauthenticated LAN HTTP surface is retired.

**Advertisement is the permission system.** A peer instantiates only node
types it advertises — placement is pull-shaped; nothing can be pushed onto a
peer, only requested of it. A peer is sovereign over three published lists:
**run** (advertised node types), **serve** (provided datasets), **subscribe**
(live addresses it follows). Selective advertisement is the sandbox: a
browser peer simply doesn't advertise shell-shaped nodes. Namespaces
organize; only vocabulary and refs are ever gated — never the data model.

**The escalation.** A *graph* arriving in-mesh is bounded composition of what
the receiver advertised: flows freely. A *native plugin* (the LLM codegen
loop: write C++ to cross-compile to ship to live-load) is new capability
injection: a distinct speech act, gated by **provenance policy** ("load
plugins whose provenance chains to sources/signers I trust") — plugins are
datasets with signed recipes, so the gate needs no new machinery.

**Scale posture.** The mechanisms are the Web's and the package manager's
trust models already: advertisement = a declarative, per-relationship
browser sandbox; provenance = signed, reproducible packages
(re-derive-and-compare beats trust). What generalizes is only membership:
one flat domain becomes the innermost of graded circles (devices /
collaborators / world). Transport is scale-dependent (mDNS + query fan-out in
the household; DHT-style discovery at world scale); semantics are not.
Deferred deliberately: per-dataset ACLs, encryption at rest, CRDTs,
multi-writer refs (single-writer-per-ref discipline until real
collaborators). Secrecy, when it comes, is decoder scarcity (keys are
decoders) — a kind, not a mechanism.

## Requirements

**msh.keypairs_pairing** Peers generate keypairs; pairing exchanges
and records acceptance; revocation removes a key and severs its sessions.
- msh.keypairs_pairing.pair_revoke_restore: three-peer pairing; revoke the Quest's key on host; Quest can no
  longer fetch, place, or subscribe; re-pairing restores.

**msh.authenticated_transport** All inter-peer traffic (control, store
fetch, net mappings) is authenticated and encrypted under the pairing keys;
no unauthenticated listener remains.
- msh.authenticated_transport.unpaired_refused: port scan + protocol probe from an unpaired host on the LAN: every
  surface refuses; the legacy HTTP behavior is demonstrably gone.

**msh.three_lists** Advertisement of run/serve/subscribe is explicit,
queryable, and enforced at request time.
- msh.three_lists.typed_refusal: a request to instantiate `shell_exec` on the browser peer (which
  doesn't advertise it) is refused with a typed error, and the refusal is
  visible to the requesting engine graph (compilation falls through to
  another peer or reports unplaceable).

**msh.pull_shaped_placement** There is no code path that instantiates a
node type on a peer absent from that peer's run list.
- msh.pull_shaped_placement.no_unadvertised_instantiation: fuzz the placement API with arbitrary type names; zero
  instantiations occur outside the advertised set (assert via registry
  audit log).

**msh.graphs_vs_plugins** Graph datasets flow and realize in-mesh
without prompts; plugin datasets require the provenance-policy gate before
dlopen/side-module load.
- msh.graphs_vs_plugins.plugin_gate: ship a graph Quest to host: runs. Ship an unsigned .so: refused,
  logged. Sign it with a paired key whose policy is accepted: loads
  hot (existing dlopen hot-reload machinery), and its provenance (source
  dataset, toolchain) is queryable.
- msh.graphs_vs_plugins.wasm_same_gate: the browser peer's plugin form is a WASM side module over the
  same channel and gate; the policy check is form-agnostic.

**msh.capture_testimony_keys** Every capture's testimony carries the
capturing peer's public key; verification is signature-checking, not trust.
- msh.capture_testimony_keys.testimony_tamper_fails: tamper with a take's testimony peer-id; verification fails.

**msh.discovery** mDNS presence + direct holdings/capability queries
serve the household scale; the discovery interface is abstract enough that a
DHT provider could replace it without touching semantics.
- msh.discovery.discovery_swappable: all MSH/STO/PKG integration tests pass with discovery swapped for
  a static peer list (proving the seam).

**msh.graded_circles** Sharing is configurable per store
graph and per advertisement list; the flat domain is expressed as "everything
shared with every paired key," not hardcoded.
- msh.graded_circles.per_store_sharing: configure a second store graph shared with a subset of keys; the
  excluded peer's fetch is refused for those hashes but succeeds for the
  common store.

## Worked example (test seed)

The codegen loop under law: an agent peer authors a new node type's C++
(a capture — human/agent testimony), a worker derivation cross-compiles it
(recipe provenance), the artifact ships by hash, the Quest's policy verifies
the chain (msh.graphs_vs_plugins.plugin_gate, msh.capture_testimony_keys.testimony_tamper_fails) and hot-loads it, and hello-cosine gains a new
node type mid-performance — no restart (the guiding star, exercised across
the trust boundary).
