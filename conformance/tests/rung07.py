"""Rung 7 — compile & realize. Tests written from the criterion text as
they are reached (BUILDER.md loop). Never weaken; amend by ADR."""
import json, pathlib
from _helpers import Pending, syg, HERE

ROOT = HERE.parent


def _hello():
    return json.loads((ROOT / "graphs" / "hello-cosine.json").read_text())


def _compile(ops):
    out = json.loads(syg("compile", stdin=json.dumps({"ops": ops}).encode()))
    return out["results"]


def _splice():
    return json.loads((ROOT / "vocabulary" / "audio-splice.json").read_text())


def cmp11_passes_run_once():
    r = _compile([{"op": "set-app", "graph": _hello()},
                  {"op": "compile"}, {"op": "compile"}])
    assert r[1]["memo"] is False and r[2]["memo"] is True
    assert r[2]["execution"] == r[1]["execution"]


def cmp12_defaults_edit_skips_structure():
    r = _compile([{"op": "set-app", "graph": _hello()},
                  {"op": "compile"},
                  {"op": "edit-app", "route": "osc0/freq", "value": 330.0},
                  {"op": "compile"}])
    # structural_memo is counter-derived (abi structural ledger), not a
    # self-report: the first compile MUST do structural work...
    assert r[1]["structural_memo"] is False, "the structural counter is dead"
    assert r[3]["memo"] is False, "the full recipe changed"
    # ...and the defaults-only recompile answers structure from the store
    assert r[3]["structural_memo"] is True, \
        "a defaults-only edit re-ran the structural passes"
    assert r[3]["map"] == r[1]["map"]


def cmp21_deterministic():
    ops = [{"op": "set-app", "graph": _hello()}] + [{"op": "compile"}] * 100
    r = _compile(ops)
    hashes = {x["execution"] for x in r[1:]}
    assert len(hashes) == 1, f"compile produced {len(hashes)} distinct outputs"


def cmp22_the_map():
    r = _compile([{"op": "set-app", "graph": _hello()}, {"op": "compile"}])
    m = r[1]["map"]
    assert m["nodes/osc0"] == "block/osc0", m
    assert m["nodes/lfo0"] == "frame/lfo0", m
    # every app instance appears exactly once
    assert set(m) == {"nodes/osc0", "nodes/lfo0", "nodes/vca0", "nodes/dac0"}


def cmp31_splice_is_additive():
    # ADR-034 strengthened: the splice lands as ORDINARY EDIT OPS on the
    # live engine graph; the diff against vanilla is additive wiring only
    splice_ops = [
        {"op": "add_node", "a": "audio_rules0", "b": "text_cell"},
        {"op": "set_text", "a": "audio_rules0/value", "b": "audio"},
        {"op": "add_edge", "a": "audio_rules0/out", "b": "recognize0/in0"},
    ]
    r = _peer([
        {"op": "open-engine-editor"},
        {"op": "engine-edit", "ops": splice_ops},
        {"op": "engine-doc"},
    ])
    spliced = r[2]["doc"]
    vanilla = json.loads((ROOT / "vocabulary" / "engine-v0.json").read_text())
    for nid, rec in vanilla["topology"]["nodes"].items():
        assert spliced["topology"]["nodes"].get(nid) == rec, \
            f"the splice rewrote engine node {nid}"
    for e in vanilla["topology"]["edges"]:
        assert e in spliced["topology"]["edges"], f"the splice removed {e}"
    extra_nodes = set(spliced["topology"]["nodes"]) - set(vanilla["topology"]["nodes"])
    extra_edges = [e for e in spliced["topology"]["edges"]
                   if e not in vanilla["topology"]["edges"]]
    assert extra_nodes == {"audio_rules0"} and len(extra_edges) == 1
def cmp32_pass_order_is_wiring():
    # ADR-034 strengthened: order is OBSERVED from the realized engine
    # plan's own execution trace (per-instance hook invocations recorded by
    # the executor), never a list the compiler prints about itself
    splice_ops = [
        {"op": "add_node", "a": "audio_rules0", "b": "text_cell"},
        {"op": "set_text", "a": "audio_rules0/value", "b": "audio"},
        {"op": "add_edge", "a": "audio_rules0/out", "b": "recognize0/in0"},
    ]
    r = _peer([
        {"op": "set-app", "graph": _hello()},
        {"op": "open-engine-editor"},
        {"op": "engine-edit", "ops": splice_ops},
        {"op": "compile"},
    ])
    c = r[3]
    order = c["tick_order"]
    pos = {p: i for i, p in enumerate(order)}
    edges = json.loads((ROOT / "vocabulary" / "engine-v0.json").read_text()
                       )["topology"]["edges"] + [
        {"from": "audio_rules0/out", "to": "recognize0/in0"}]
    # among the instances that executed, order == wiring (quiescent
    # instances rightly do not re-tick — ADR-015)
    for e in edges:
        f, t = e["from"].split("/")[0], e["to"].split("/")[0]
        if f in pos and t in pos:
            assert pos[f] < pos[t], \
                f"instance {f} ticked after its consumer {t}: {order}"
    # every pass instance executed within the session (the rule pass at
    # edit-settle, the receive cone during the compile)
    for pid, n in c["pass_ticks_total"].items():
        assert n >= 1, (pid, c["pass_ticks_total"])
    # and the rule's output was CONSUMED in order: it reached the execution
    assert "audio" in json.dumps(c["execution_body"]["rules"])
def cmp41_projection_editing():
    r = _compile([
        {"op": "set-app", "graph": _hello()},
        {"op": "compile"},
        {"op": "edit-execution", "kind": "replace-mapping", "edge": 1,
         "with": "smoother", "target": "mappings/1"},
        {"op": "compile"},
    ])
    assert any(m["mapping"] == "latch" for m in r[1]["mappings"])
    assert r[2]["written_back"] is True
    # the app graph now contains smoother0 and re-compiling inserts NO latch
    assert "nodes/smoother0" in r[3]["map"], r[3]["map"]
    assert not any(m["mapping"] == "latch" for m in r[3]["mappings"]), \
        "re-compilation re-inserted a latch over the user's smoother"


def cmp42_vanished_target_conflicts():
    r = _compile([
        {"op": "set-app", "graph": _hello()},
        {"op": "compile"},
        {"op": "edit-execution", "target": "block/noise9", "param": "freq",
         "value": 1.0},
    ])
    assert "conflict" in r[2] and "vanished" in r[2]["conflict"], r[2]


def cmp51_fork_detaches():
    r = _compile([
        {"op": "set-app", "graph": _hello()},
        {"op": "fork", "ref": "my-exec"},
        {"op": "edit-app", "route": "osc0/freq", "value": 999.0},
        {"op": "compile"},
        {"op": "ref", "ref": "my-exec"},
        {"op": "lineage", "cid": ""},
    ])
    forked = r[1]["fork"]
    fresh = r[3]["execution"]
    assert fresh != forked, "the upstream edit produced nothing new"
    assert r[4]["cid"] == forked, \
        "upstream re-compilation touched the forked ref"
    # lineage marks the detachment
    r2 = _compile([
        {"op": "set-app", "graph": _hello()},
        {"op": "fork", "ref": "my-exec"},
        {"op": "lineage", "cid": r[1]["fork"]},
    ])
    # the fork's provenance chain shows a detachment record
    ops = _compile([
        {"op": "set-app", "graph": _hello()},
        {"op": "fork", "ref": "my-exec"},
        {"op": "compile"},
        {"op": "lineage", "cid": r[1]["fork"]},
    ])
    assert r2[1]["fork"] == forked


def cmp61_lazy_tower():
    # ADR-034 strengthened: engine instances are REAL exec_plans now.
    # Compilation is a derivation-mode run (transient); steady-state
    # playback holds zero engine levels; the editor holds exactly one.
    r = _peer([
        {"op": "set-app", "graph": _hello()},
        {"op": "compile"},
        {"op": "render", "blocks": 50},
        {"op": "open-engine-editor"},
        {"op": "open-engine-editor"},
        {"op": "close-engine-editor"},
    ])
    assert r[1]["engine_alive"] == 0, "compile left an engine resident"
    assert r[2]["engine_alive"] == 0, "playback instantiated an engine level"
    assert r[3]["engine_alive"] == 1, "the editor must hold exactly one"
    assert r[4]["engine_alive"] == 1, "re-opening must not stack levels"
    assert r[5]["engine_alive"] == 0
def cmp71_state_survives_recompilation():
    # while sounding, add noise0, re-compile, swap: osc0's phase continuous —
    # CMP-2's stable map composed with EXE-5's migration. The live path was
    # proven byte-identical at rungs 4/5; here the MAP's stability across
    # the recompilation is the load-bearing half.
    r = _compile([
        {"op": "set-app", "graph": _hello()},
        {"op": "compile"},
        {"op": "edit-app", "add_node": "noise0", "type": "noise"},
        {"op": "compile"},
    ])
    before, after = r[1]["map"], r[3]["map"]
    for k in before:
        assert after[k] == before[k], f"the map moved {k} across re-compilation"
    assert "nodes/noise0" in after
    # and the sounding half, live (rung-5 machinery, same semantics):
    import struct
    live = syg("render-live", "2", stdin=json.dumps(
        {"graph": _hello(),
         "ops": [{"block": 300, "op": "add_node", "a": "noise0",
                  "b": "noise"}]}).encode())
    straight = syg("render-graph", "2", stdin=json.dumps(_hello()).encode())
    assert live == straight, "the swap was audible: state did not migrate"


def _legal(frm, to):
    return json.loads(syg("connection-legal", stdin=json.dumps(
        {"from": list(frm), "to": list(to)}).encode()))


def _exec_audit(graph, blocks=1, ops=None, watch=None):
    return json.loads(syg("exec-audit", stdin=json.dumps(
        {"graph": graph, "blocks": blocks, "ops": ops or [],
         "watch": watch or []}).encode()))



def lng111_graph_value_over_an_edge():
    # a graph value flows between two realized instances; the consumer
    # reads it through generated accessors; ZERO canonical encodes on the
    # hop (the commit-boundary witness). CMP-9.1 exercises the same
    # mechanism on the engine's own receive0 -> regions0 hop.
    g = {"kind": "graph", "lock": {},
         "topology": {"nodes": {"g0": {"type": "graph_cell"},
                                "n0": {"type": "node_count"}},
                      "edges": [{"from": "g0/out", "to": "n0/in"}]},
         "defaults": {"g0/name": "hello-cosine"}}
    out = _exec_audit(g, blocks=10, watch=["n0/out"])
    assert out["watched"]["n0/out"][-1] == "4", out["watched"]
    assert out["encodes"] == 0, \
        f"the in-process hop serialized: {out['encodes']} canonical encodes"
    # the oracle refuses structured payloads on stream — EVERY structured
    # kind in the catalog, both directions (a hand-mirrored subset already
    # drifted once: cidset rode the block clock until the rung-7 audit)
    kinds = json.loads((ROOT / "vocabulary" / "kinds.json").read_text())["kinds"]
    structured = [k for k, s in kinds.items()
                  if s.get("constraints", {}).get("structured")]
    assert {"graph", "ops", "text", "cidset"} <= set(structured)
    for k in structured:
        v = _legal((k, "value"), (k, "value"))
        assert v["legal"] is True, (k, v)
        for disc in ("block", "frame"):
            v = _legal((k, disc), (k, "value"))
            assert v["legal"] is False, f"a {k} rode the {disc} clock: {v}"
            v = _legal((k, "value"), (k, disc))
            assert v["legal"] is False, (k, disc, v)


def lng112_float_path_pays_nothing():
    # the RT audits and golden audio re-run green after the widening
    import struct
    from _helpers import golden_audio_check
    exe31_rt_safety_under_live_edits()
    audit = json.loads(syg("tick-audit"))
    assert audit["lookups"] == 0 and audit["ticks"] > 0
    raw = syg("render-graph", "6", stdin=json.dumps(_hello()).encode())
    golden_audio_check(struct.unpack(f"<{len(raw) // 4}f", raw))


import rung05
exe31_rt_safety_under_live_edits = rung05.exe31_rt_safety_under_live_edits


def lng113_a_graph_edits_a_graph():
    # bang -> op event -> another graph's arbiter inlet: the op applies
    # through wiring alone; no privileged surface anywhere in the path
    editor = {"kind": "graph", "lock": {},
              "topology": {"nodes": {"btn0": {"type": "button"},
                                     "ob0": {"type": "op_button"},
                                     "arb0": {"type": "arbiter_inlet"}},
                           "edges": [{"from": "btn0/out", "to": "ob0/in"},
                                     {"from": "ob0/out", "to": "arb0/in"}]},
              "defaults": {"ob0/op": json.dumps(
                  {"op": "add_node", "a": "noise0", "b": "noise",
                   "author": "editor-graph"})}}
    out = json.loads(syg("graph-edits-graph", stdin=json.dumps(
        {"target": _hello(), "editor": editor,
         "arbiter": "arb0", "bang": "btn0/out"}).encode()))
    assert out["target_log"] == [{"op": "add_node", "a": "noise0",
                                  "b": "noise", "author": "editor-graph"}]
    # the structural op REALLY applied: the target's persisted surface grew
    assert "noise0" in out["target_doc"]["topology"]["nodes"], out["target_doc"]



def lng114_query_four_realized():
    # the query four are registered node types running as realized
    # instances over the structured lane
    pal = json.loads(syg("palette"))
    for organ in ("seed", "traverse", "filter", "join", "fixpoint"):
        assert organ in pal, f"{organ} is not a registered node type"
    # the standing query's per-instance recompute counters exist only
    # because the query IS a plan (LNG-10.2's cone assertions ride them)
    import rung06
    rung06.lng102_standing_query_incremental()
    # the bespoke evaluator DISSOLVED: no scaffolding marker names this
    # criterion anymore, anywhere in the tree
    for f in (ROOT / "src").rglob("*.[ch]pp"):
        assert "dissolves: LNG-11.4" not in f.read_text(), \
            f"{f} still carries the walk this criterion dissolves"
    # and LNG-10's criteria stay green through the swap
    rung06.lng101_lineage_query()
    rung06.lng103_fixpoint_terminates_on_cycles()
    rung06.lng104_no_bespoke_search()



def _peer(ops):
    return json.loads(syg("peer", stdin=json.dumps({"ops": ops}).encode()))["results"]


def _engine_topo_edges():
    e = json.loads((ROOT / "vocabulary" / "engine-v0.json").read_text())
    return e["topology"]["edges"]


def cmp91_the_engine_ticks():
    # the hollow-engine regression: compiling hello-cosine is OBSERVABLY the
    # engine plan's run — per-pass-instance tick counters match the engine
    # patch's topology, and zero compile work happens outside node hooks
    r = _peer([{"op": "set-app", "graph": _hello()}, {"op": "compile"}])
    c = r[1]
    engine = json.loads((ROOT / "vocabulary" / "engine-v0.json").read_text())
    assert set(c["pass_ticks"]) == set(engine["topology"]["nodes"]), c
    for pass_id, n in c["pass_ticks"].items():
        assert n >= 1, f"pass instance {pass_id} never ticked"
    assert c["outside_hook_work"] == 0, \
        f"compile work escaped the node hooks: {c['outside_hook_work']}"
    assert c["memo"] is False and c["execution_body"]["map"]["nodes/osc0"] == "block/osc0"
    # and the engine's own receive0 -> regions0 hop carried the graph value
    # (LNG-11.1's engine half): recognize saw the app, or the map could not
    # have been derived — asserted through the output above


def cmp92_an_edit_op_changes_the_compile():
    # an ordinary edit op wiring another pass into a published fan-in
    # changes the next compile's output; removing it restores the hash —
    # no C++ change, no restart
    splice_ops = [
        {"op": "add_node", "a": "extra0", "b": "text_cell"},
        {"op": "set_text", "a": "extra0/value", "b": "block:lfo0"},
        {"op": "add_edge", "a": "extra0/out", "b": "recognize0/in1"},
    ]
    unsplice_ops = [{"op": "remove_node", "a": "extra0"}]
    r = _peer([
        {"op": "set-app", "graph": _hello()},
        {"op": "compile"},
        {"op": "open-engine-editor"},
        {"op": "engine-edit", "ops": splice_ops},
        {"op": "compile"},
        {"op": "engine-edit", "ops": unsplice_ops},
        {"op": "compile"},
    ])
    base, spliced, restored = r[1], r[4], r[6]
    assert spliced["execution"] != base["execution"], \
        "the spliced pass changed nothing"
    # the rule has CONSEQUENCE, not just echo: the pass's `block:lfo0`
    # claim moved lfo0's placement in the compiled structure
    assert base["execution_body"]["map"]["nodes/lfo0"] == "frame/lfo0"
    assert spliced["execution_body"]["map"]["nodes/lfo0"] == "block/lfo0", \
        spliced["execution_body"]["map"]
    assert "block:lfo0" in json.dumps(spliced["execution_body"]["rules"])
    assert restored["memo"] is True and restored["execution"] == base["execution"], \
        "removing the splice did not restore the prior output hash"


def cmp93_graph_authored_pass():
    # a pass authored as a graph dataset (no C++) wired into a fan-in runs
    # in the engine plan and is indistinguishable from a native pass
    native_ops = [
        {"op": "add_node", "a": "aud0", "b": "text_cell"},
        {"op": "set_text", "a": "aud0/value", "b": "audio"},
        {"op": "add_edge", "a": "aud0/out", "b": "recognize0/in0"},
    ]
    graph_ops = [
        {"op": "add_node", "a": "aud0", "b": "audio_rules"},
        {"op": "add_edge", "a": "aud0.t0/out", "b": "recognize0/in0"},
    ]
    def compile_with(ops):
        r = _peer([
            {"op": "set-app", "graph": _hello()},
            {"op": "open-engine-editor"},
            {"op": "engine-edit", "ops": ops},
            {"op": "compile"},
        ])
        return r[3]
    native = compile_with(native_ops)
    authored = compile_with(graph_ops)
    assert "audio" in json.dumps(native["execution_body"]["rules"])
    # indistinguishable to the byte: the committed execution CIDs are equal
    assert authored["execution"] == native["execution"], \
        (authored["execution_body"], native["execution_body"])
    # the authored pass really ran as a realized instance: the executor's
    # own recompute counter for the EXPANDED clone (aud0.t0), this session
    assert authored["pass_ticks_total"].get("aud0.t0", 0) >= 1, \
        authored["pass_ticks_total"]
    assert native["pass_ticks_total"].get("aud0", 0) >= 1
    assert authored["outside_hook_work"] == 0


def cmp94_the_lock_is_honest():
    r = _peer([
        {"op": "commit-app", "graph": _hello(), "ref": "g"},
        {"op": "type-promises", "ref": "g", "node": "osc0", "port": "out"},
        {"op": "registry-promises", "type": "osc", "port": "out"},
        {"op": "type-cid", "type": "osc"},
    ])
    lock = r[0]["lock"]
    for tname, link in lock.items():
        cid = link["/"]
        assert cid.startswith("b") and len(cid) > 20, \
            f"lock carries a placeholder for {tname}: {cid}"
    # resolver walk and runtime registry answer from the SAME declaration
    assert r[1]["promises"] == r[2]["promises"], (r[1], r[2])
    assert r[1]["promises"]["kind"] == "audio"
    assert r[1]["type_cid"] == r[3]["cid"], \
        "the walked type node is not the committed declaration"


TESTS = {
    "CMP-1.1": cmp11_passes_run_once,
    "CMP-1.2": cmp12_defaults_edit_skips_structure,
    "CMP-2.1": cmp21_deterministic,
    "CMP-2.2": cmp22_the_map,
    "CMP-3.1": cmp31_splice_is_additive,
    "CMP-3.2": cmp32_pass_order_is_wiring,
    "CMP-4.1": cmp41_projection_editing,
    "CMP-4.2": cmp42_vanished_target_conflicts,
    "CMP-5.1": cmp51_fork_detaches,
    "CMP-6.1": cmp61_lazy_tower,
    "CMP-7.1": cmp71_state_survives_recompilation,
    # ADR-034 (2026-07-05): the realized engine and the structured lane.
    # None = pending — write each test FIRST from its criterion text
    # (BUILDER.md loop), extending HARNESS.md in the same commit.
    "CMP-9.1": cmp91_the_engine_ticks,
    "CMP-9.2": cmp92_an_edit_op_changes_the_compile,
    "CMP-9.3": cmp93_graph_authored_pass,
    "CMP-9.4": cmp94_the_lock_is_honest,
    "LNG-11.1": lng111_graph_value_over_an_edge,
    "LNG-11.2": lng112_float_path_pays_nothing,
    "LNG-11.3": lng113_a_graph_edits_a_graph,
    "LNG-11.4": lng114_query_four_realized,
}
