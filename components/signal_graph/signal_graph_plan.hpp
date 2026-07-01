// Copyright 2026 Travis West
#pragma once
#include <deque>
#include <optional>

#include "port_types.hpp"
#include "signal_graph.hpp"

// Edge executor (planning/edge_executor.design.md): ONE plan for the whole
// graph, ticked by per-region schedulers. True edges are references
// resolved once against the owning region's value store; edges that close
// cycles are reified z⁻¹ delay mappings; edges that cross regions are
// reified boundary mappings chosen by port_types::crossing_mapping.
//
// Regions (v1): Block = `dac` nodes + upstream closure through audio
// edges, ticked by the audio scheduler; everything else is Frame (render).
// (The pure rate-quotient inference from the design is deliberately
// narrowed: a node with audio IN but no audio OUT — spectrogram — pins
// render via its other ports and consumes the stream through a ring.)

// One resolved connection: the PRODUCER's owned storage → the target
// node. Resolved at plan build; read_output() does the typed read.
// (Phase D: there is no values map — references go to node storage.)
struct EdgeApplier {
    const Edge* edge;
    const NodeInstance* from = nullptr;  // producer instance
    std::string kind;                    // producer port kind
};

// A cycle-closing edge: applies last tick's captured value (z⁻¹).
// prev stays empty until the first capture, so tick 1 behaves exactly as
// an unconnected input — the certified feedback semantics.
struct DelayMapping {
    EdgeApplier applier;
    std::optional<PortValue> prev;
    port_types::Rate region = port_types::Rate::Frame;
};

// A region-crossing edge, to be mediated by the named canonical mapping
// (ring / latch / snapshot / queue). The audio scheduler instantiates
// the mapping state; the plan only declares it.
struct Crossing {
    const Edge* edge;
    std::string payload_kind;
    std::string_view mapping;
    port_types::Rate from_region, to_region;
};

// An excess-rank edge resolved as a LIFT (conformability.md "match cells,
// lift frames"): the producer emits a Span whose cell matches the consumer's
// scalar/vector input kind, so the consumer is replicated along the array's
// leading axis. The schedulers replay this instead of a wire.
//
// Strategy is chosen from the host descriptor's lift_kind:
//   Clones  — stateful default: N migrated host instances, one row each.
//             Builtins and subgraphs alike via desc->create()/destroy()
//             (a subgraph's create() already clone_graphs its template).
//   CellMap — stateless: loop the ONE host's process over rows, no per-row
//             state (L2).
//   Scan    — Time axis: deferred (the audio Lift stamp covers audio).
struct LiftGroup {
    std::size_t host = 0;  // consumer node index in g.nodes
    std::string in_port;   // the lifted input port
    EdgeApplier source;    // producer Span
    int cell = 0;          // lifted input cell rank (src cols match)
    enum Strategy { Clones, CellMap, Scan } strategy = Clones;
    std::string out_port;  // host output gathered downstream
    std::string out_kind;  // its kind
    int out_cell = 0;      // gathered output cell rank (numeric)
    std::string key_port;  // lift_key input (keyed identity) or empty
    // Keyed by a SEPARATE source (vr_editor_decomposition.md S4): when
    // key_port != in_port, an independent Span edge into key_port carries the
    // key (e.g. graph_source.keys → card.id), so a card keys on its stable id,
    // not the lifted position cell. Empty source ⇒ key on the lifted cell
    // (the L1 case, key_port == in_port).
    EdgeApplier key_source;
    int key_cell = 0;  // key Span cols
    // Clones: live host instances, migrated across ticks (kept by key/index
    // so a lifted instance KEEPS ITS STATE when the array reorders).
    std::vector<NodeInstance> instances;
    std::vector<std::string> inst_keys;  // parallel: each instance's key
    // Gather: lifted outputs concatenate into a plan-owned slot the
    // downstream consumer's src points at (mirrors audio_slots). Span for
    // numeric cells; mesh_gather for an N-mesh payload (forest route 1).
    Span gather;
    std::vector<float> gather_buf;
    std::vector<MeshPtr> mesh_gather;  // out_kind == "mesh" (forest route 1)
    MeshArray mesh_view;               // view onto mesh_gather, repointed/tick
};

struct TickPlan {
    std::vector<port_types::Rate> node_region;        // per node index
    std::vector<std::size_t> order;                   // Frame, DAG order
    std::vector<std::size_t> block_order;             // Block, DAG order
    std::vector<std::vector<EdgeApplier>> appliers;   // same-region true edges
    std::vector<std::vector<DelayMapping*>> delayed;  // per target node
    std::deque<DelayMapping> delays;                  // owned; stable addresses
    std::vector<Crossing> crossings;
    // endpoints v6: same-region non-delayed edges between v6-capable nodes
    // become literal pointers (consumer src → producer storage), wired once
    // by wire_plan. No applier, no per-tick copy.
    std::vector<const Edge*> wires;
    // Region-crossing audio edges into a v6 consumer: the ring mapping
    // drains into a PLAN-OWNED slot the consumer's src points at —
    // "cross-region consumers point at the MAPPING's slot", literally.
    // deque: stable addresses.
    struct SlotApplier {
        EdgeApplier applier;
        AudioBuffer* audio = nullptr;
    };
    std::deque<AudioBuffer> audio_slots;
    std::vector<std::vector<SlotApplier>> slot_appliers;  // per target node
    // Excess-rank edges resolved as lifts. deque: stable addresses (gather
    // slots are pointed at by downstream consumers' src).
    std::deque<LiftGroup> lift_groups;
    // Edges whose producer is a lifted host's gathered output: the consumer
    // points its src at the LiftGroup's gather Span (mesh_gather meshes are
    // delivered by tick). Resolved in wire_plan after gather is sized.
    struct GatherWire {
        const Edge* edge;
        LiftGroup* group;
    };
    std::vector<GatherWire> gather_wires;
};

std::unique_ptr<TickPlan> build_plan(const Graph& g);

// endpoints v6: reset every v6 input's src, then point plan.wires at their
// producers' storage. Call after build_plan, post-migration (migrated
// instances carry STALE src pointers into destroyed producers otherwise).
void wire_plan(Graph& g);

// Which edges are currently mediated by an auto-inserted z⁻¹? (editor /
// serialization visibility; recomputed, never persisted)
std::vector<const Edge*> cycle_mappings(const Graph& g);

// Shared executor primitives (one implementation for every scheduler —
// render, block, subgraph, net proxies).
void apply_value(const NodeInstance& n, const char* port, const PortValue& value);
// Output context whose emit callbacks write into `store` (observability
// sweeps and tests; execution never touches a store).
EyeballsOutputCtx output_ctx(
    std::unordered_map<std::string, PortValue>* store, const char* node_id);
// Typed read of an applier's producer storage (output_ptr + kind).
std::optional<PortValue> read_output(const EdgeApplier& a);
// Typed read of an arbitrary output port (crossing endpoints, probes).
std::optional<PortValue> read_output(
    const NodeInstance& n, const std::string& port, const std::string& kind);
