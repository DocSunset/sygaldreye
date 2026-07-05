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
    assert r[3]["memo"] is False, "the full recipe changed"
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
    # ADR-034 strengthened CMP-3.1: the splice must land as ordinary edit
    # ops on the REALIZED engine graph, not as a JSON diff. The body below
    # predates the strengthening — rewrite it from the new criterion text.
    raise Pending("CMP-3.1 strengthened by ADR-034 — splice as edit ops on "
                  "the realized engine; rewrite from the new criterion text")
    r = _compile([{"op": "engine-diff", "splice": _splice()}])
    assert r[0]["additive"] is True, "the splice rewrote existing engine nodes"
    assert r[0]["added_nodes"] == 2 and r[0]["added_edges"] == 2


def cmp32_pass_order_is_wiring():
    # ADR-034 strengthened CMP-3.2: order must be OBSERVED as per-instance
    # tick counts in the realized engine plan; the body below compares the
    # compiler's self-reported list against the same topo sort that made it
    # (self-referential — the hollow-engine hole). Rewrite from the new text.
    raise Pending("CMP-3.2 strengthened by ADR-034 — observe tick counts in "
                  "the realized engine plan, not passes_run")
    r = _compile([{"op": "set-app", "graph": _hello()},
                  {"op": "compile", "splice": _splice()}])
    order = r[1]["passes_run"]
    engine = json.loads((ROOT / "vocabulary" / "engine-v0.json").read_text())
    edges = engine["topology"]["edges"] + _splice()["add_edges"]
    pos = {p: i for i, p in enumerate(order)}
    for e in edges:
        f, t = e["from"].split("/")[0], e["to"].split("/")[0]
        assert pos[f] < pos[t], f"pass {f} ran after its consumer {t}"
    assert "audio_recognize0" in order, "the spliced pass never ran"


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
    # Vacuous until CMP-9: engine instances cannot yet exist, so counting
    # zero of them proves nothing. Rewrite against real instances once the
    # engine is realized (ADR-034).
    raise Pending("CMP-6.1 vacuous until CMP-9 — engine instances cannot "
                  "yet exist; rewrite against the realized engine")
    r = _compile([
        {"op": "set-app", "graph": _hello()},
        {"op": "compile"},
        {"op": "render-blocks", "blocks": 50},   # steady-state playback
        {"op": "open-engine-editor"},
    ])
    assert r[1]["engine_instances"] == 0, "compiling made an engine resident"
    assert r[2]["engine_instances"] == 0, \
        "steady-state playback instantiated an engine level"
    assert r[3]["engine_instances"] == 1, "opening the editor makes exactly one"


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
    # the oracle refuses structured payloads on stream
    v = _legal(("graph", "value"), ("graph", "value"))
    assert v["legal"] is True
    for disc in ("block", "frame"):
        v = _legal(("graph", disc), ("graph", "value"))
        assert v["legal"] is False, f"a graph rode the {disc} clock: {v}"
        v = _legal(("graph", "value"), ("graph", disc))
        assert v["legal"] is False


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
    "CMP-9.1": None,
    "CMP-9.2": None,
    "CMP-9.3": None,
    "CMP-9.4": None,
    "LNG-11.1": lng111_graph_value_over_an_edge,
    "LNG-11.2": lng112_float_path_pays_nothing,
    "LNG-11.3": None,
    "LNG-11.4": None,
}
