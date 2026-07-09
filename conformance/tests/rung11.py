"""Rung 11 — surfaces (the editor, store browser, documents). Tests written
from the criterion text as reached (BUILDER.md loop). The editor is graphs
editing graphs: a live plan whose arbiter takes edit ops exactly as the
gesture nodes emit them. Never weaken; amend the book by ADR."""
import json
from _helpers import Pending, syg, HERE

ROOT = HERE.parent


def _hello():
    return json.loads((ROOT / "graphs" / "hello-cosine.json").read_text())


def _edit(ops):
    out = syg("edit", stdin=json.dumps({"ops": ops}).encode())
    return json.loads(out)["results"]


# ---- edr.defaults_discipline.default_is_fallback: serialize captures the default, never a live sample ----------
def edr21_defaults_not_live_values():
    # vca0/gain is a CONNECTED inlet — modulated by lfo0 (a live cell swinging
    # every block). Editing it updates the fallback DEFAULT; save/reload keeps
    # that default, never a captured LFO sample (exe.plan_cache.saves_default at the UX level).
    g = _hello()
    assert {"from": "lfo0/out", "to": "vca0/gain"} in g["topology"]["edges"]
    r = _edit([
        {"op": "open", "graph": g},
        # drag the modulated inlet's fallback to a distinctive value
        {"op": "gesture", "ops": [{"op": "set_param", "a": "vca0/gain", "b": "0.7"}],
         "settle": 32},                          # let the LFO swing many blocks
        {"op": "save"},
    ])
    saved = r[2]["graph"]["defaults"]
    assert saved["vca0/gain"] == 0.7, saved       # the edit, not an LFO sample
    # reload the saved graph: the default survives the round-trip
    r2 = _edit([{"op": "open", "graph": r[2]["graph"]}, {"op": "save"}])
    assert r2[1]["graph"]["defaults"]["vca0/gain"] == 0.7, r2[1]["graph"]


# ---- edr.undo.undo_sequence_restores: structural-snapshot undo restores topology + defaults --------
def edr31_undo_restores_exactly():
    # [add node, drag param, remove node, undo, undo] restores the original
    # topology AND defaults exactly. Undo snapshots on STRUCTURAL change; the
    # param drag folds into the surrounding step (param drift never trashes
    # history), so two undos unwind the two structural changes to the origin.
    g = _hello()
    base = _edit([{"op": "open", "graph": g}, {"op": "save"}])[1]["graph"]
    r = _edit([
        {"op": "open", "graph": g},
        {"op": "gesture", "ops": [{"op": "add_node", "a": "noise9", "b": "noise"}]},
        {"op": "gesture", "ops": [{"op": "set_param", "a": "osc0/freq", "b": "440"}]},
        {"op": "gesture", "ops": [{"op": "remove_node", "a": "noise9"}]},
        {"op": "undo", "n": 1},   # reverts the remove: {orig + noise9 + freq=440}
        {"op": "undo", "n": 1},   # reverts the add (folding the freq drag): origin
        {"op": "save"},
    ])
    after = r[-1]["graph"]
    assert after == base, {"base": base, "after": after}
    # one undo alone lands on the intermediate structural snapshot, not origin
    mid = _edit([
        {"op": "open", "graph": g},
        {"op": "gesture", "ops": [{"op": "add_node", "a": "noise9", "b": "noise"}]},
        {"op": "gesture", "ops": [{"op": "set_param", "a": "osc0/freq", "b": "440"}]},
        {"op": "gesture", "ops": [{"op": "remove_node", "a": "noise9"}]},
        {"op": "undo", "n": 1},
        {"op": "save"},
    ])[-1]["graph"]
    assert mid != base, "one undo must not already be at the origin"
    assert "noise9" in mid["topology"]["nodes"], "undo of remove restores the node"
    assert mid["defaults"].get("osc0/freq") == 440.0, mid["defaults"]


# ---- edr.editor_is_nodes.palette_swap_live: the palette is a subgraph, swappable live --------------------
def edr11_palette_swap_changes_behavior():
    # The editor is nodes: its palette is a subgraph that, banged, emits an
    # add_node op into a target's arbiter (graphs editing graphs). Swapping
    # the palette SUBGRAPH live (slot swap+migrate) changes what it spawns —
    # no restart, the same live plan keeps ticking.
    editor = {"kind": "graph", "lock": {},
              "topology": {"nodes": {"btn": {"type": "button"},
                                     "pal0": {"type": "palette_osc"},
                                     "arb0": {"type": "arbiter_inlet"}},
                           "edges": [{"from": "btn/out", "to": "pal0/bang"},
                                     {"from": "pal0/out", "to": "arb0/in"}]},
              "defaults": {}}
    r = _edit([
        {"op": "open-editor", "editor": editor, "target": _hello(),
         "arbiter": "arb0"},
        {"op": "bang", "route": "btn/out"},               # palette_osc
        {"op": "target-doc"},
        {"op": "swap", "id": "pal0", "type": "palette_noise"},  # live swap
        {"op": "bang", "route": "btn/out"},               # palette_noise
        {"op": "target-doc"},
    ])
    before = r[2]["graph"]["topology"]["nodes"]
    after = r[5]["graph"]["topology"]["nodes"]
    # the original palette spawned an osc, and only that
    assert before.get("osc_spawn", {}).get("type") == "osc", before
    assert "noise_spawn" not in before, before
    # after the live swap the SAME editor now spawns a noise — behavior
    # changed without restart; the earlier spawn survived the migrate
    assert after.get("noise_spawn", {}).get("type") == "noise", after
    assert after.get("osc_spawn", {}).get("type") == "osc", "migrate lost state"


# ---- edr.realized_view_editing.gesture_replaces_latch: gesture-driven smoother replacement (cmp.projection_editing.writeback_smoother) ----------------
def edr41_smoother_replacement_gesture():
    # The realized view shows hello-cosine's auto latch as compiler-inserted
    # and replaceable. A scripted gesture (agent-driven through the editor's
    # own op_button source nodes, per edr.agents_as_peers) replaces it with a smoother; the
    # write-back is app-graph edit ops, and re-compilation inserts NO latch —
    # the user's smoother survives (cmp.projection_editing.writeback_smoother).
    hello = _hello()
    before = _edit([{"op": "realized", "graph": hello}])[0]["mappings"]
    # the latch is a compiler-DERIVED boundary adapter (present in the derived
    # mappings list) — that presence IS what "compiler-inserted" means; the
    # label is derived from the mapping's kind, not stamped
    latch = [m for m in before if m["mapping"] == "latch"]
    assert latch, before
    assert latch[0]["compiler_inserted"] and latch[0]["replaceable"], latch[0]
    assert latch[0]["label"] == "compiler-inserted latch", latch[0]

    # the replacement, driven agent-source through op_button gesture nodes
    script = [
        {"op": "add_node", "a": "smoother0", "b": "smoother", "author": "agent"},
        {"op": "remove_edge", "a": "lfo0/out", "b": "vca0/gain", "author": "agent"},
        {"op": "add_edge", "a": "lfo0/out", "b": "smoother0/in", "author": "agent"},
        {"op": "add_edge", "a": "smoother0/out", "b": "vca0/gain", "author": "agent"},
    ]
    editor = {"kind": "graph", "lock": {},
              "topology": {"nodes": {"btn": {"type": "button"},
                                     "ob0": {"type": "op_button"},
                                     "arb0": {"type": "arbiter_inlet"}},
                           "edges": [{"from": "btn/out", "to": "ob0/in"},
                                     {"from": "ob0/out", "to": "arb0/in"}]},
              "defaults": {}}
    edited = _edit([{"op": "agent-drive", "editor": editor, "target": hello,
                     "arbiter": "arb0", "op_route": "ob0/op", "bang": "btn/out",
                     "script": script}])[0]["graph"]
    assert "smoother0" in edited["topology"]["nodes"], edited

    # re-compile the edited app: the smoother is now the mapping, no latch
    after = _edit([{"op": "realized", "graph": edited}])[0]["mappings"]
    assert not any(m["mapping"] == "latch" for m in after), \
        f"re-compilation re-inserted a latch over the smoother: {after}"


# ---- edr.agents_as_peers.human_agent_identical: human-input and agent-source yield identical graphs ----------
def edr71_human_and_agent_identical():
    # Every editor gesture is drivable through source nodes; there is no
    # privileged agent API. The same gesture suite, run two ways — human-input
    # simulation (direct arbiter gestures) and agent-source driving (each op
    # arriving as an op event from an op_button source node) — produces
    # byte-identical resulting graphs.
    script = [
        {"op": "add_node", "a": "g0", "b": "noise", "author": "hand"},
        {"op": "add_edge", "a": "g0/out", "b": "vca0/in", "author": "hand"},
        {"op": "add_node", "a": "g1", "b": "lfo", "author": "hand"},
        {"op": "set_param", "a": "osc0/freq", "b": "330", "author": "hand"},
        {"op": "set_param", "a": "g1/depth", "b": "0.5", "author": "hand"},
        {"op": "remove_node", "a": "g0", "author": "hand"},
    ]
    # human-input simulation: the gestures applied directly to the arbiter
    human = _edit([{"op": "open", "graph": _hello()}]
                  + [{"op": "gesture", "ops": [g]} for g in script]
                  + [{"op": "save"}])[-1]["graph"]

    # agent-source: the SAME gestures driven through op_button source nodes
    editor = {"kind": "graph", "lock": {},
              "topology": {"nodes": {"btn": {"type": "button"},
                                     "ob0": {"type": "op_button"},
                                     "arb0": {"type": "arbiter_inlet"}},
                           "edges": [{"from": "btn/out", "to": "ob0/in"},
                                     {"from": "ob0/out", "to": "arb0/in"}]},
              "defaults": {}}
    agent = _edit([{"op": "agent-drive", "editor": editor, "target": _hello(),
                    "arbiter": "arb0", "op_route": "ob0/op", "bang": "btn/out",
                    "script": script}])[0]["graph"]

    assert human == agent, {"human": human, "agent": agent}
    # and the suite actually did non-trivial work (not both no-ops)
    assert "g1" in human["topology"]["nodes"], human       # add survived
    assert "g0" not in human["topology"]["nodes"], human    # remove survived
    assert human["defaults"]["osc0/freq"] == 330.0, human   # param drag survived
    assert human["defaults"]["g1/depth"] == 0.5, human      # added-node param


def _walk(store, ops):
    out = syg("walk", stdin=json.dumps({"store": store, "ops": ops}).encode())
    return json.loads(out)["results"]


# ---- edr.documents.live_vs_fixed_transclusion: transclusion — live subscribes, fixed quotes -----------------
def edr61_live_vs_fixed_transclusion():
    # A document transcluding an address LIVE (refname:route) updates when the
    # ref moves; the same address FIXED (cid/route, normalized) does not. Live
    # is subscription, fixed is quotation (ch. 9, ADR-029).
    import copy
    h1 = _hello()
    h2 = copy.deepcopy(h1)
    h2["defaults"]["osc0/freq"] = 440.0    # a new version of the same graph
    route = ["defaults", "osc0/freq"]
    cid1 = _walk({"graphs": {"graphs/hello-cosine": h1}},
                 [{"op": "cid-of", "ref": "graphs/hello-cosine"}])[0]["cid"]
    r = _walk({"graphs": {"graphs/hello-cosine": h1}}, [
        {"op": "transclude", "ref": "graphs/hello-cosine", "route": route},  # 0 live v1
        {"op": "transclude", "cid": cid1, "route": route},                   # 1 fixed v1
        {"op": "rebind", "ref": "graphs/hello-cosine", "graph": h2},          # 2 ref moves
        {"op": "transclude", "ref": "graphs/hello-cosine", "route": route},  # 3 live v2
        {"op": "transclude", "cid": cid1, "route": route},                   # 4 fixed v1
    ])
    assert r[0]["value"] == 220.0 and r[0]["mode"] == "live", r[0]
    assert r[1]["value"] == 220.0 and r[1]["mode"] == "fixed", r[1]
    # after the ref moves: the LIVE transclusion updates, the FIXED one holds
    assert r[3]["value"] == 440.0, "live transclusion did not follow the ref"
    assert r[4]["value"] == 220.0, "fixed transclusion moved with the ref"


# ---- edr.documents.cpp_roundtrip: the C++ round-trip metric over a corpus ----------------------
def edr62_cpp_roundtrip_corpus():
    # Decompose a small permissive C++ file to document form (top-level segment
    # nodes linked in sequence) and regenerate byte-identically. A property
    # test over a corpus directory — each file is a real decomposition (more
    # than one segment) that reconstructs to the exact original bytes.
    corpus = sorted((ROOT / "conformance" / "fixtures" / "cpp_corpus").glob("*.cpp"))
    assert corpus, "the C++ corpus is missing"
    for f in corpus:
        src = f.read_text()
        r = syg("document", stdin=json.dumps(
            {"ops": [{"op": "roundtrip", "source": src}]}).encode())
        res = json.loads(r)["results"][0]
        assert res["identical"], f"{f.name} did not round-trip byte-identically"
        assert res["regen_len"] == len(src.encode()), (f.name, res["regen_len"])
        # a real decomposition, not verbatim whole-file storage
        assert res["segments"] > 1, f"{f.name} was not decomposed into parts"


# ---- edr.store_browser.walk_and_mark: the store browser walks + marks -------------------------------
def edr51_walk_and_mark():
    # here/path/frontier/mark over a store graph. Walk ground -> the
    # hello-cosine ref -> topology -> nodes -> osc0 -> its type -> the osc type
    # node -> ports (the committed lock->type->ports chain). Mark osc0; the
    # marked map persists as a dataset and re-opens.
    store = {"graphs": {"graphs/hello-cosine": _hello()}}
    r = _walk(store, [
        {"op": "ground"},
        {"op": "walk", "path": ["graphs/hello-cosine", "topology", "nodes",
                                "osc0", "type", "osc", "ports"]},
        {"op": "mark", "as": "marks/osc",
         "paths": [["graphs/hello-cosine", "topology", "nodes", "osc0"]],
         "links": [{"note": "the oscillator"}]},
        {"op": "open-mark", "ref": "marks/osc"},
    ])
    # ground's frontier reaches the named ref
    assert any(s["name"] == "graphs/hello-cosine"
               for s in r[0]["frontier"]["steps"]), r[0]
    # the walk lands on the osc type node's ports (the committed chain worked)
    ports = {s["name"] for s in r[1]["frontier"]["steps"]}
    assert {"freq", "shape", "out"} <= ports, ports
    # the mark persisted and re-opens as a dataset
    m = r[3]["map"]
    assert m["kind"] == "marked-map", m
    assert m["marks"] == [["graphs/hello-cosine", "topology", "nodes", "osc0"]], m
    assert m["links"] == [{"note": "the oscillator"}], m


# ---- edr.store_browser.frontier_paginates: frontier paginates over 100k links ---------------------------
def edr52_frontier_paginates():
    # A node with 100,000 links: the frontier is demand-driven — each page is
    # bounded, the pages are disjoint and cover, and no call ever returns the
    # whole set (the frame region is never blocked by materializing 100k).
    store = {"fanout": {"big": 100000}}
    a = _walk(store, [{"op": "frontier", "path": ["big"], "page": 0, "page_size": 100}])[0]
    assert a["total"] == 100000, a["total"]
    assert len(a["steps"]) == 100, len(a["steps"])   # a bounded page, not 100k
    b = _walk(store, [{"op": "frontier", "path": ["big"], "page": 500, "page_size": 100}])[0]
    assert len(b["steps"]) == 100, b
    # pages are disjoint (different windows of the same frontier)
    pa = {s["name"] for s in a["steps"]}
    pb = {s["name"] for s in b["steps"]}
    assert pa.isdisjoint(pb), "pages overlap — pagination is not windowed"
    # every entry is a real content-addressed link, not a placeholder
    assert all(s["kind"] == "link" for s in a["steps"]), a["steps"][:3]


# ---- edr.observability.probe_any_edge: an edge probe exposes the value stream, regions unaltered ----
def edr81_probe_does_not_move_regions():
    # A probe attached to an edge exposes its value stream on the values
    # surface (pull-observability) WITHOUT altering region inference. The probe
    # is a subscription, not a splice: the topology is untouched, so the block
    # and frame partitions are identical to the un-probed graph.
    g = _hello()
    r = _edit([{"op": "probe", "graph": g, "edges": ["lfo0/out"], "blocks": 200}])[0]
    base, probed = r["base"], r["probed"]
    probe_ids = set(r["probe_ids"])
    # the probe is a REAL inert observer present in the probed region
    # computation — so this witness has teeth: a demanding probe would land in
    # block (pulling its source with it) and this would fail.
    assert probe_ids and probe_ids <= set(probed["inert"]), (probe_ids, probed)
    # the ORIGINAL nodes' partition is untouched: block is byte-identical, and
    # frame differs ONLY by the added inert probe
    assert sorted(probed["block"]) == sorted(base["block"]), (base, probed)
    assert sorted(set(probed["frame"]) - probe_ids) == sorted(base["frame"]), \
        (base, probed)
    # nothing got pulled into the block region by the attachment
    assert not (set(probed["block"]) - set(base["block"])), "probe moved a region"
    # and the probe carries a REAL, MOVING value stream (the LFO swings) in range
    lfo = r["streams"]["lfo0/out"]
    assert len(lfo) == 200 and len(set(round(x, 4) for x in lfo)) > 5, lfo[:5]
    assert all(-1.001 <= x <= 1.001 for x in lfo), "LFO out of range"


def edr12_surface_is_graphs():
    # edr.editor_is_nodes.surface_is_graphs: every editor-surface behavior lives in graphs/editor/*.json;
    # the registered natives exactly equal vocabulary/packages.json; no
    # source file under src/nodes implements an editor-surface concept
    import json as _json, re as _re
    ed = ROOT / "graphs" / "editor" / "editor.json"
    if not ed.exists():
        raise Pending("graphs/editor/ not authored yet "
                      "(planning/embodiment_plan.md Phase C)")
    pkgs = _json.loads((ROOT / "vocabulary" / "packages.json").read_text())
    declared = {n for names in pkgs["packages"].values() for n in names}
    registered = set(_json.loads(syg("palette")))
    assert registered == declared, \
        f"native set drifted from packages.json: {sorted(registered ^ declared)}"
    banned = _re.compile(
        r"card|palette_mesh|wire_drag|slider_drag|dwell|handle_pick|gesture_",
        _re.IGNORECASE)
    hits = []
    for f in sorted((ROOT / "src" / "nodes").rglob("*")):
        if f.suffix not in {".cpp", ".hpp"}:
            continue
        for i, ln in enumerate(f.read_text().splitlines(), 1):
            if banned.search(ln):
                hits.append(f"{f.relative_to(ROOT)}:{i}")
    assert not hits, f"editor-surface concept in native source (law.graphs_first): {hits}"
    # every surface graph parses through the persisted surface (lng.round_trip)
    for gf in sorted((ROOT / "graphs" / "editor").glob("*.json")):
        syg("roundtrip", stdin=gf.read_bytes())


TESTS = {
    "edr.editor_is_nodes.palette_swap_live": edr11_palette_swap_changes_behavior,
    "edr.editor_is_nodes.surface_is_graphs": edr12_surface_is_graphs,
    "edr.defaults_discipline.default_is_fallback": edr21_defaults_not_live_values,
    "edr.undo.undo_sequence_restores": edr31_undo_restores_exactly,
    "edr.store_browser.walk_and_mark": edr51_walk_and_mark,
    "edr.store_browser.frontier_paginates": edr52_frontier_paginates,
    "edr.realized_view_editing.gesture_replaces_latch": edr41_smoother_replacement_gesture,
    "edr.documents.live_vs_fixed_transclusion": edr61_live_vs_fixed_transclusion,
    "edr.documents.cpp_roundtrip": edr62_cpp_roundtrip_corpus,
    "edr.agents_as_peers.human_agent_identical": edr71_human_and_agent_identical,
    # edr.agents_as_peers.author_from_scratch pends until Phase C pins the palette layout the
    # pointer script must hit (embodiment plan)
    "edr.agents_as_peers.author_from_scratch": None,
    "edr.observability.probe_any_edge": edr81_probe_does_not_move_regions,
}
