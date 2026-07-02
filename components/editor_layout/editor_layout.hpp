// Copyright 2026 Travis West
#pragma once
#include <cstdint>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "event_queue.hpp"
#include "signal_graph.hpp"

// The editor's spatial model as ONE shared, testable function (S3/S5,
// vr_editor_decomposition.md): given the live graph + the per-id card-position
// overrides, lay out every card, port handle and scalar slider in world space.
// Ported verbatim from the VrEditor monolith (vr_editor.cpp default grid +
// vr_editor_handles.cpp left/right columns, 0.018 m row pitch, scalar→slider)
// so the decomposed gesture nodes hit-test identical geometry and the card
// subgraph renders at identical positions.
//
// Identity travels WITH the geometry here (node id + port name as std::string),
// which is the simplest, most general identity resolution: a gesture node reads
// this layout off the graph it observes (a resource-holder, like graph_source)
// and turns a nearest-hit into an edit op — no text-array payload needed.

namespace editor_layout {

inline constexpr float kCardSpacing = 0.45f;
inline constexpr float kCardW = 0.4f;
inline constexpr float kBaseCardH = 0.08f;
inline constexpr float kPortRowH = 0.018f;
inline constexpr float kHandleSize = 0.012f;
inline constexpr float kHandleOffX = 0.01f;
inline constexpr float kHandleHalf = 0.006f;

struct Handle {
    std::string node_id;
    std::string port_name;
    std::string port_kind;
    bool is_output = false;
    Eigen::Vector3f world_pos{0, 0, 0};
};

struct Slider {
    std::string node_id;
    std::string port_name;
    Eigen::Vector3f world_pos{0, 0, 0};
    float width = 0.f;
    float min_val = 0.f;
    float max_val = 1.f;
    float value = 0.f;
};

struct Card {
    std::string node_id;
    Eigen::Vector3f position{0, 0, 0};
    float width = kCardW;
    float height = kBaseCardH;
    std::vector<Handle> inputs;
    std::vector<Handle> outputs;
    std::vector<Slider> sliders;
};

struct Layout {
    std::vector<Card> cards;
};

// The controller tip: 5 cm in front of the pose along its forward (-z), the
// poke point the monolith hit-tests with (vr_editor.cpp).
inline Eigen::Vector3f controller_tip(const Eigen::Vector3f& pos, const Eigen::Quaternionf& rot) {
    Eigen::Quaternionf q = (rot.norm() > 1e-6f) ? rot.normalized() : Eigen::Quaternionf::Identity();
    return pos + q * Eigen::Vector3f{0.f, 0.f, -0.05f};
}

// FNV-1a id hash → the same key graph_source publishes (24-bit, exact in a
// float). Lets a layout hit resolve to the key column when needed.
float id_key(const std::string& id);

// Default card grid: a reachable band, 8 per row, 4 rows, then a fresh block
// to the right (mirrors VrEditor / graph_source default_pos).
Eigen::Vector3f default_card_pos(int i);

// Build the whole editor layout from a graph: one card per node, handles per
// wirable port, sliders per scalar input (value seeded from live params).
// `overrides`: per-id dragged card position (graph_source.pos_ovr).
Layout build_layout(
    const Graph& g, const std::unordered_map<std::string, Eigen::Vector3f>& overrides);

// Edge endpoint world positions (for wire/edge hit-testing). Returns false if
// either endpoint handle is absent.
bool edge_endpoints(const Layout& l, const Edge& e, Eigen::Vector3f& from, Eigen::Vector3f& to);

// One memoized layout shared by every editor node in a frame (7 nodes used to
// rebuild it independently, each re-serializing every slider node). Owned by
// the host (peer_core), keyed on (graph, graph_gen, overrides_gen).
struct LayoutCache {
    Layout layout;
    const Graph* graph = nullptr;
    std::uint64_t graph_gen = 0;
    std::uint64_t overrides_gen = 0;
    bool valid = false;
    std::uint64_t builds = 0;  // test observability
};

// The context the host offers every node through the generic descriptor seam
// (ABI v9 set_host_context, kind "editor"): the live graph to hit-test, the
// edit queue to emit ops, the shared per-id card-position overrides
// (graph_source reads, wire_drag writes), the sorted registry type list
// (palette + spawner), the registry itself, and the shared layout cache with
// its generation stamps. A consuming node is a resource-holder (it observes
// the graph it lives in), exactly like graph_source/edit_sink.
using PosOverrides = std::unordered_map<std::string, Eigen::Vector3f>;
struct GestureContext {
    const Graph* graph = nullptr;
    EventQueue<std::string>* edits = nullptr;
    PosOverrides* overrides = nullptr;
    const std::vector<std::string>* types = nullptr;  // sorted registry types (palette)
    const ComponentRegistry* registry = nullptr;      // spawner: type lookup
    LayoutCache* layout = nullptr;                    // host-owned memoized layout
    std::uint64_t graph_gen = 0;      // bumped on swap AND on in-place param writes
    std::uint64_t* overrides_gen = nullptr;  // bump when writing `overrides`
};

inline constexpr const char* kEditorContextKind = "editor";

// The shared layout, rebuilt only when (graph, graph_gen, overrides_gen)
// moved. Without a cache in the context it rebuilds every call (tests).
const Layout& cached_layout(const GestureContext& ctx);

// Escape a string for interpolation into edit-op JSON (ids/ports/types are
// data, not syntax). apply_edit_op unescapes on the parse side.
std::string json_escape(std::string_view s);

}  // namespace editor_layout
