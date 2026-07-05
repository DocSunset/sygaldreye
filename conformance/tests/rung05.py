"""Rung 5 — execution semantics. Tests written from the criterion text as
they are reached (BUILDER.md loop). Never weaken a test; amend by ADR."""
import json, pathlib
from _helpers import Pending, syg, HERE

ROOT = HERE.parent


def _regions(graph, edits=None):
    return json.loads(syg("regions", stdin=json.dumps(
        {"graph": graph, "edits": edits or []}).encode()))


def _hello():
    return json.loads((ROOT / "graphs" / "hello-cosine.json").read_text())


def exe21_hello_regions():
    r = _regions(_hello())
    assert sorted(r["block"]) == ["dac0", "osc0", "vca0"], r
    assert r["frame"] == ["lfo0"], r
    assert r["errors"] == []


def exe22_regions_recompute_per_edit():
    # deleting edge 2 (vca0 -> dac0) moves osc0/vca0 out of the block
    # region on the very next edit application
    r = _regions(_hello(), edits=[{"op": "remove_edge", "index": 2}])
    assert r["block"] == ["dac0"], r
    assert sorted(r["frame"]) == ["lfo0", "osc0", "vca0"], r


def _exec_audit(graph, blocks=1, ops=None, watch=None):
    return json.loads(syg("exec-audit", stdin=json.dumps(
        {"graph": graph, "blocks": blocks, "ops": ops or [],
         "watch": watch or []}).encode()))


def _const_hello(with_lfo=True):
    # osc at freq 0, cosine shape = a constant 1.0 source, so the dac input
    # IS the gain — block-boundary semantics become directly observable
    g = _hello()
    g["defaults"]["osc0/freq"] = 0.0
    if not with_lfo:
        del g["topology"]["nodes"]["lfo0"]
        g["topology"]["edges"] = [e for e in g["topology"]["edges"]
                                  if not e["from"].startswith("lfo0")]
        del g["defaults"]["lfo0/freq"]
        g["defaults"]["vca0/gain"] = 0.3
    return g


def exe11_plan_cache():
    # true edges resolve to raw pointers; the latch is DERIVED — visible in
    # the regions/mappings view, never in the persisted topology
    r = _regions(_hello())
    mapped = {m["edge"] for m in r["mappings"]}
    assert mapped == {1}, r
    assert r["mappings"][0]["mapping"] == "latch"
    out = _exec_audit(_hello(), blocks=8)
    assert "mappings" not in json.dumps(out["serialized"]), \
        "derived structure leaked into the persisted surface"
    assert out["serialized"] == _hello(), "serialize is not the source graph"


def exe12_defaults_never_capture_modulation():
    # run mid-modulation, then serialize: gain's DEFAULT persists, not the
    # LFO's current sample
    out = _exec_audit(_hello(), blocks=100)
    assert out["serialized"]["defaults"] == _hello()["defaults"]


def exe31_rt_safety_under_live_edits():
    ops = [{"block": k, "route": "lfo0/freq", "value": str(0.5 + (k % 2) / 10)}
           for k in range(10000)]
    out = _exec_audit(_hello(), blocks=10000, ops=ops)
    assert out["rt_events"] == 0, \
        f"allocations/locks on the block path: {out['rt_events']}"


def exe41_latch_at_block_start():
    # (a) with the LFO waving, the gain the block side sees is constant
    # WITHIN every block (the latch applies at block start, never mid-block)
    out = _exec_audit(_const_hello(), blocks=200, watch=["out"])
    firsts = [s[0] for s in out["watched"]["out"]]
    for first, last in out["watched"]["out"][2:]:
        assert first == last, "gain moved mid-block"
    assert len(set(firsts)) > 10, "the lfo never moved the latch"
    # (b) a param step lands exactly at the next block boundary
    out = _exec_audit(_const_hello(with_lfo=False), blocks=8, watch=["out"],
                      ops=[{"block": 4, "route": "vca0/gain", "value": "0.7"}])
    w = out["watched"]["out"]
    for blk, (first, last) in enumerate(w):
        assert first == last, "step landed mid-block"
        # ops queued for block k drain at k's OPENING boundary; the step
        # is whole-block from there, and never earlier
        want = 0.3 if blk < 4 else 0.7
        assert abs(first - want) < 1e-6, (blk, first)


def exe62_pump_offline():
    # no audio device anywhere in the harness: the offline pump paces the
    # block region and hello-cosine still computes
    import struct
    raw = syg("render-graph", "2", stdin=json.dumps(_hello()).encode())
    x = struct.unpack(f"<{len(raw) // 4}f", raw)
    assert len(x) == 2 * 48000 // 128 * 128
    assert max(abs(v) for v in x) > 0.1, "headless peer stopped computing"


def _value_chain():
    return {
        "kind": "graph",
        "lock": {"cell": "kind:cell@v1", "scale": "kind:scale@v1"},
        "topology": {
            "nodes": {"c0": {"type": "cell"}, "s1": {"type": "scale"},
                      "s2": {"type": "scale"}, "c1": {"type": "cell"},
                      "s3": {"type": "scale"}},
            "edges": [{"from": "c0/out", "to": "s1/in"},
                      {"from": "s1/out", "to": "s2/in"},
                      {"from": "c1/out", "to": "s3/in"}]},
        "defaults": {"c0/k": 2.0, "s1/k": 3.0, "s2/k": 5.0,
                     "c1/k": 7.0, "s3/k": 11.0}}


def exe111_static_scene_quiesces():
    # >1,000 frames with no clock-wired nodes and no events: every value
    # node computed once (settling) and never again — while still presenting
    out = _exec_audit(_value_chain(), blocks=6600, watch=["s2/out"])
    for node, n in out["recomputes"].items():
        assert n <= 2, f"{node} recomputed {n} times in a static scene"
    assert abs(out["watched"]["s2/out"][-1] - 2.0 * 3.0 * 5.0) < 1e-6


def exe112_dirty_cone_exactly():
    # one param write recomputes exactly the dirty downstream cone
    blocks = 130
    base = _exec_audit(_value_chain(), blocks=blocks)["recomputes"]
    out2 = _exec_audit(_value_chain(), blocks=blocks,
                       ops=[{"block": 60, "route": "c0/k", "value": "4"}])
    delta = {k: out2["recomputes"][k] - base[k] for k in base}
    assert delta["c0"] == 1 and delta["s1"] == 1 and delta["s2"] == 1, delta
    assert delta["c1"] == 0 and delta["s3"] == 0, \
        f"recompute leaked outside the cone: {delta}"


def exe113_inert_lint():
    # a node with no demanded output and no clock input is flagged
    r = _regions(_value_chain())
    assert sorted(r["inert"]) == ["c0", "c1", "s1", "s2", "s3"], r
    r = _regions(_hello())
    assert r["inert"] == [], "lfo0 is clock-wired and demanded — not inert"


TESTS = {
    "EXE-1.1": exe11_plan_cache,
    "EXE-1.2": exe12_defaults_never_capture_modulation,
    "EXE-2.1": exe21_hello_regions,
    "EXE-2.2": exe22_regions_recompute_per_edit,
    "EXE-3.1": exe31_rt_safety_under_live_edits,
    "EXE-4.1": exe41_latch_at_block_start,
    "EXE-4.2": None,
    "EXE-4.3": None,
    "EXE-6.1": None,
    "EXE-6.2": exe62_pump_offline,
    "EXE-7.1": None,
    "EXE-8.1": None,
    "EXE-9.1": None,
    "EXE-10.1": None,
    "EXE-10.2": None,
    "EXE-10.3": None,
    "EXE-10.4": None,
    "EXE-11.1": exe111_static_scene_quiesces,
    "EXE-11.2": exe112_dirty_cone_exactly,
    "EXE-11.3": exe113_inert_lint,
    "EXE-11.4": None,
    "LNG-1.1": None,
    "LNG-2.1": None,
    "LNG-2.2": None,
    "LNG-3.1": None,
    "LNG-4.1": None,
    "LNG-4.2": None,
    "LNG-5.1": None,
    "LNG-5.2": None,
    "LNG-6.1": None,
    "LNG-6.2": None,
    "LNG-7.1": None,
    "LNG-9": None,
    "TCF-1": None,
    "TCF-2": None,
    "TCF-3": None,
    "TCF-4": None,
    "TCF-5": None,
}
