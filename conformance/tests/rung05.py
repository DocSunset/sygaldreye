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



def exe61_no_singleton_reach():
    # no ::instance() reach from node code into engine machinery
    for d in ("nodes", "organs"):
        base = ROOT / "src" / d
        for f in list(base.rglob("*.cpp")) + list(base.rglob("*.hpp")):
            assert "::instance()" not in f.read_text(), f


def tcf3_clock_honesty():
    # no wall-clock syscalls from any node; kernels receive dt/increments
    import re
    banned = re.compile(r"\btime\(|gettimeofday|clock_gettime|system_clock")
    for d in ("nodes", "organs", "escapement", "crown", "executor"):
        base = ROOT / "src" / d
        if not base.exists():
            continue
        for f in list(base.rglob("*.cpp")) + list(base.rglob("*.hpp")):
            assert not banned.search(f.read_text()), f"wall-clock in {f}"


def tcf5_movement_austerity():
    # a frozen movement build contains no fault package, no heartbeats, no
    # dirty bookkeeping — symbol audit proves absence
    import shutil, subprocess
    firmware = ROOT / "build" / "hello-cosine-movement"
    if not firmware.exists() or not shutil.which("nm"):
        raise Pending("sealed movement binary or nm unavailable")
    syms = subprocess.run(["nm", "-C", str(firmware)], capture_output=True,
                          text=True).stdout.lower()
    for banned in ("fault", "heartbeat", "dirty", "quarantine", "supervisor",
                   "crown", "arbiter"):
        assert banned not in syms, f"movement carries '{banned}' machinery"


def lng9_text_events_still_open():
    # LNG-9 is OPEN by design: until text events are ratified, no vocabulary
    # may declare a text-kind event port — text inlets are persistence-only
    gen = ROOT / "build" / "generated"
    if not gen.exists():
        raise Pending("generated descriptors not built")
    for f in gen.glob("*.descriptor.json"):
        for pn, pv in json.loads(f.read_text())["ports"].items():
            assert not (pv["kind"] == "text" and pv["discipline"] == "event"), \
                f"{f.name}:{pn} declares the unratified text event payload"



def exe91_existence_is_reference():
    # removing lfo0 (its edge cascades in the one op) collects its state;
    # the severed consumer falls back to its inlet default (silence-shaped);
    # undo restores from the structural snapshot, not the live cell
    g = _const_hello()  # constant-1 osc: the output IS the gain
    ops = [{"block": 50, "op": "remove_node", "a": "lfo0"},
           {"block": 100, "op": "undo"}]
    out = _exec_audit(g, blocks=150, ops=ops, watch=["out"])
    w = out["watched"]["out"]
    assert len({s[0] for s in w[10:49]}) > 5, "lfo was not modulating"
    assert all(s[0] == 0.0 for s in w[51:99]), \
        "severed gain did not fall back to its default"
    assert len({s[0] for s in w[105:149]}) > 5, "undo did not restore lfo0"
    # the one remove op recorded its full inverse (node + edge + defaults)
    rec = [e for e in out["log"] if e["op"] == "remove_node"]
    assert rec and rec[0]["inverse_ops"] >= 3, rec


def lng51_ops_replay_to_same_hash():
    ops = [{"block": 3, "route": "osc0/freq", "value": "330"},
           {"block": 5, "op": "add_node", "a": "noise0", "b": "noise"},
           {"block": 7, "op": "add_edge", "a": "noise0/out", "b": "vca0/in"},
           {"block": 8, "op": "remove_edge", "a": "noise0/out", "b": "vca0/in"},
           {"block": 9, "op": "remove_node", "a": "noise0"}]
    a = _exec_audit(_hello(), blocks=12, ops=ops)["serialized"]
    b = _exec_audit(_hello(), blocks=12, ops=ops)["serialized"]
    ha = syg("hash", stdin=syg("encode", stdin=json.dumps(a).encode()))
    hb = syg("hash", stdin=syg("encode", stdin=json.dumps(b).encode()))
    assert ha == hb, "replaying the session's ops diverged"
    # and the replayed session actually went somewhere and came back
    assert a["topology"]["nodes"] == _hello()["topology"]["nodes"]


def lng52_ops_carry_authors():
    out = _exec_audit(_hello(), blocks=4, ops=[
        {"block": 1, "route": "osc0/freq", "value": "330",
         "author": "z6MkTravis"},
        {"block": 2, "op": "add_node", "a": "noise0", "b": "noise",
         "author": "z6MkAgent"}])
    authors = {e["op"]: e["author"] for e in out["log"]}
    assert authors["set_param"] == "z6MkTravis", out["log"]
    assert authors["add_node"] == "z6MkAgent", out["log"]



def _ks(delay_extra=None, spectro_in_loop=False):
    g = {"kind": "graph",
         "lock": {"pulse": "kind:pulse@v1", "add": "kind:add@v1",
                  "delay": "kind:delay@v1", "vca": "kind:vca@v1",
                  "dac": "kind:dac@v1"},
         "topology": {
             "nodes": {"pulse0": {"type": "pulse"}, "add0": {"type": "add"},
                       "delay0": {"type": "delay"}, "fb0": {"type": "vca"},
                       "dac0": {"type": "dac"}},
             "edges": [{"from": "pulse0/out", "to": "add0/a"},
                       {"from": "fb0/out", "to": "add0/b"},
                       {"from": "add0/out", "to": "delay0/in"},
                       {"from": "delay0/out", "to": "fb0/in"},
                       {"from": "add0/out", "to": "dac0/in"}]},
         "defaults": {"delay0/samples": 99.0, "fb0/gain": 0.995}}
    if delay_extra is not None:
        # an explicit extra delay inside the loop (delay0 -> blk0 -> fb0)
        g["topology"]["nodes"]["blk0"] = {"type": "delay"}
        g["topology"]["edges"] = [
            e for e in g["topology"]["edges"]
            if not (e["from"] == "delay0/out" and e["to"] == "fb0/in")]
        g["topology"]["edges"] += [{"from": "delay0/out", "to": "blk0/in"},
                                   {"from": "blk0/out", "to": "fb0/in"}]
        g["defaults"]["blk0/samples"] = float(delay_extra)
        g["lock"]["delay"] = "kind:delay@v1"
    if spectro_in_loop:
        g["topology"]["nodes"]["spec0"] = {"type": "spectro"}
        g["topology"]["edges"].append({"from": "add0/out", "to": "spec0/in"})
        # drag the spectro INTO the cycle: feed something from... spectro has
        # no outs; instead wire the loop THROUGH a block-override by making
        # spec0 sit inside: emulate with an edge forming a cycle via spec0 is
        # impossible (no outs) — use a self-looped block-override instead
        g["lock"]["spectro"] = "kind:spectro@v1"
    return g


def _peak_hz(x, candidates, sr=48000):
    import math
    def goertzel(f):
        w = 2 * math.pi * f / sr
        c = 2 * math.cos(w)
        s1 = s2 = 0.0
        for v in x:
            s0 = v + c * s1 - s2
            s2, s1 = s1, s0
        return math.sqrt(abs(s1 * s1 + s2 * s2 - c * s1 * s2))
    return max(candidates, key=goertzel)


def exe101_island_pitch():
    import struct
    raw = syg("render-graph", "1", stdin=json.dumps(_ks()).encode())
    x = struct.unpack(f"<{len(raw) // 4}f", raw)
    # 1-sample feedback math: f = 48000/(99+1) = 480 Hz;
    # block-delayed math would ring at 48000/(99+128) ≈ 211 Hz
    assert _peak_hz(x, [480.0, 48000 / (99 + 128), 375.0]) == 480.0
    assert max(abs(v) for v in x) <= 1.0


def exe102_explicit_delay_opts_out():
    # per-callback kernel-call count: interleaved island = per-node-per-
    # sample; an explicit block-sized delay reverts to per-node-per-block
    island = _exec_audit(_ks(), blocks=10)["process_calls"]
    cut = _exec_audit(_ks(delay_extra=128), blocks=10)["process_calls"]
    assert island > 10 * 3 * 100, island   # 3 island nodes stepped per sample
    assert cut <= 10 * 6, cut              # fused: one call per node per block


def exe103_block_override_in_cycle_rejected():
    # a spectrogram tap OUTSIDE the loop is legal; rewiring the loop
    # THROUGH the block-override yields an edit-time error naming the edge
    g = _ks()
    g["lock"]["spectro"] = "kind:spectro@v1"
    g["topology"]["nodes"]["spec0"] = {"type": "spectro"}
    g["topology"]["edges"].append({"from": "add0/out", "to": "spec0/in"})
    _exec_audit(g, blocks=2)  # the tap: fine
    try:
        _exec_audit(g, blocks=2, ops=[
            {"block": 0, "op": "remove_edge", "a": "delay0/out", "b": "fb0/in"},
            {"block": 0, "op": "add_edge", "a": "delay0/out", "b": "spec0/in"},
            {"block": 0, "op": "add_edge", "a": "spec0/out", "b": "fb0/in"}])
        raise AssertionError("a cycle through a block-override was accepted")
    except AssertionError as e:
        msg = str(e)
        assert "spec0" in msg and "explicit block-sized delay" in msg, msg
        assert "->" in msg, f"the edge is not named: {msg}"


def exe104_frozen_equals_interpreted():
    # the freezer is a pure optimizer (ADR-013/014): the hand-frozen
    # loop-carried form of the KS island renders BYTE-IDENTICAL to the
    # sample-interleaved interpreter
    live = syg("render-graph", "2", stdin=json.dumps(_ks()).encode())
    frozen = syg("render-movement", "ks", "2")
    assert live == frozen, "freezing changed the sound, not just the cost"


TESTS = {
    "EXE-1.1": exe11_plan_cache,
    "EXE-1.2": exe12_defaults_never_capture_modulation,
    "EXE-2.1": exe21_hello_regions,
    "EXE-2.2": exe22_regions_recompute_per_edit,
    "EXE-3.1": exe31_rt_safety_under_live_edits,
    "EXE-4.1": exe41_latch_at_block_start,
    "EXE-4.2": None,
    "EXE-4.3": None,
    "EXE-6.1": exe61_no_singleton_reach,
    "EXE-6.2": exe62_pump_offline,
    "EXE-7.1": None,
    "EXE-8.1": None,
    "EXE-9.1": exe91_existence_is_reference,
    "EXE-10.1": exe101_island_pitch,
    "EXE-10.2": exe102_explicit_delay_opts_out,
    "EXE-10.3": exe103_block_override_in_cycle_rejected,
    "EXE-10.4": exe104_frozen_equals_interpreted,
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
    "LNG-5.1": lng51_ops_replay_to_same_hash,
    "LNG-5.2": lng52_ops_carry_authors,
    "LNG-6.1": None,
    "LNG-6.2": None,
    "LNG-7.1": None,
    "LNG-9": lng9_text_events_still_open,
    "TCF-1": None,
    "TCF-2": None,
    "TCF-3": tcf3_clock_honesty,
    "TCF-4": None,
    "TCF-5": tcf5_movement_austerity,
}
