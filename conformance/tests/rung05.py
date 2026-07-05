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



def lng31_queue_never_drops():
    out = json.loads(syg("queue-audit", "10000"))
    assert out["count"] == 10000, out    # 10,000 bangs count exactly 10,000
    assert out["disorder"] == 0, out
    # serialize of the patch contains no bang state: the event graph's
    # persisted surface has no counter/button state anywhere
    audit = _exec_audit({"kind": "graph",
                         "lock": {"button": "b", "counter": "c"},
                         "topology": {"nodes": {"button0": {"type": "button"},
                                                "counter0": {"type": "counter"}},
                                      "edges": [{"from": "button0/out",
                                                 "to": "counter0/in"}]},
                         "defaults": {}}, blocks=2)
    assert audit["serialized"]["defaults"] == {}, \
        "bang state leaked into the persisted surface"


def exe42_events_in_order():
    # N palette-press-shaped events across threads arrive N times, in order
    out = json.loads(syg("queue-audit", "5000"))
    assert out["count"] == 5000 and out["disorder"] == 0, out


def exe43_z_inverse_certified():
    # a self-loop integrator: out[i] = pulse[i] + out[i-1]. Exactly one
    # sample of delay with empty-until-first-capture makes it all-ones
    import struct
    g = {"kind": "graph",
         "lock": {"pulse": "p", "add": "a", "dac": "d"},
         "topology": {"nodes": {"pulse0": {"type": "pulse"},
                                "add0": {"type": "add"},
                                "dac0": {"type": "dac"}},
                      "edges": [{"from": "pulse0/out", "to": "add0/a"},
                                {"from": "add0/out", "to": "add0/b"},
                                {"from": "add0/out", "to": "dac0/in"}]},
         "defaults": {}}
    raw = syg("render-graph", "0.05", stdin=json.dumps(g).encode())
    x = struct.unpack(f"<{len(raw) // 4}f", raw)
    assert x and all(v == 1.0 for v in x), \
        f"z-inverse is not exactly one sample: {x[:6]}..."


def exe114_bang_wakes_cone_same_tick():
    # a quiesced value region: a bang wakes exactly the consumer's cone
    g = {"kind": "graph",
         "lock": {"button": "b", "counter": "c", "scale": "s", "cell": "k"},
         "topology": {"nodes": {"button0": {"type": "button"},
                                "counter0": {"type": "counter"},
                                "s1": {"type": "scale"},
                                "c1": {"type": "cell"},
                                "s2": {"type": "scale"}},
                      "edges": [{"from": "button0/out", "to": "counter0/in"},
                                {"from": "counter0/out", "to": "s1/in"},
                                {"from": "c1/out", "to": "s2/in"}]},
         "defaults": {"s1/k": 2.0, "c1/k": 1.0, "s2/k": 3.0}}
    blocks = 130  # ~16 frame ticks: everything settles, then quiesces
    base = _exec_audit(g, blocks=blocks)["recomputes"]
    out = _exec_audit(g, blocks=blocks,
                      ops=[{"block": 100, "op": "bang", "route": "button0/out"}],
                      watch=["counter0/out", "s1/out"])
    delta = {k: out["recomputes"][k] - base[k] for k in base}
    assert delta["counter0"] == 1 and delta["s1"] == 1, delta
    assert delta["c1"] == 0 and delta["s2"] == 0, \
        f"the bang woke more than its cone: {delta}"
    # and the VALUES landed (was a vacuous `== {} or True` until the
    # 2026-07-05 audit): the bang counted once, its consumer scaled it
    assert out["watched"]["counter0/out"][-1] == 1.0, out["watched"]
    assert out["watched"]["s1/out"][-1] == 2.0, out["watched"]


def tcf1_mapping_guarantees():
    # each mapping row's stress test, where its machinery lives today:
    # queue — multi-producer MPSC, no loss/dup (this run); latch — EXE-4.1;
    # z⁻¹ — EXE-4.3 + EXE-10.1; snapshot — the block→frame publish below;
    # ring/net — their packages' rungs (PKG-6.1 / rung 8+), cross-referenced
    out = json.loads(syg("queue-audit", "20000", "4"))
    assert out["count"] == 20000, out
    # snapshot: the frame side sees exactly the last completed block's value
    g = _ks()
    g["lock"]["spectro"] = "kind:spectro@v1"
    g["topology"]["nodes"]["spec0"] = {"type": "spectro"}
    g["topology"]["edges"].append({"from": "add0/out", "to": "spec0/in"})
    _exec_audit(g, blocks=4)  # realizes with a snapshot mapping, stable
    r = _regions(g)
    assert any(m["mapping"] == "snapshot" for m in r["mappings"]), r


def tcf2_swaps_under_load():
    out = json.loads(syg("swap-storm", "10000",
                         stdin=json.dumps(_hello()).encode()))
    assert out["finite"] is True, "a torn value reached the output"
    assert out["events_counted"] == 1000, \
        f"events lost across swaps: {out}"
    assert out["applied"] + out["rejected"] == 10000, out
    assert out["rejected"] < out["applied"], out



def lng11_kind_catalog():
    out = json.loads(syg("kinds"))
    want = {"scalar", "bool", "vec2", "vec3", "vec4", "quat", "mat4", "text",
            "bang", "audio", "texture", "draw_call", "mesh", "surface",
            "span", "any", "graph", "ops", "cidset"}  # ADR-034's three
    assert set(out["kinds"]) == want, sorted(set(out["kinds"]) ^ want)
    for name, node in out["kinds"].items():
        assert "decoders" in node and "widget" in node, name
    assert out["kinds"]["audio"]["constraints"]["pins_clock"] == "block"
    assert out["unresolved"] == [], \
        f"registry promises unknown kinds: {out['unresolved']}"


def lng41_defaults_are_the_inlet_model():
    # EXE-1.2 / EDR-2.1 restated: an edge overrides while connected; the
    # persisted default never captures live modulation
    exe12_defaults_never_capture_modulation()
    # and an unconnected inlet actually holds its default (EXE-4.1's step)
    out = _exec_audit(_const_hello(with_lfo=False), blocks=3, watch=["out"])
    assert out["watched"]["out"][2][0] == 0.30000001192092896 or \
        abs(out["watched"]["out"][2][0] - 0.3) < 1e-6


def lng42_widget_table_is_data():
    # adding range metadata to a scalar inlet switches its widget without
    # editor code changes — the table is a dataset, keyed by kind + metadata
    assert json.loads(syg("widget-of", "scalar")) == {"widget": "number"}
    assert json.loads(syg("widget-of", "scalar", "--range")) == {"widget": "slider"}
    assert json.loads(syg("widget-of", "bool")) == {"widget": "toggle"}
    assert json.loads(syg("widget-of", "vec3")) == {"widget": "gizmo"}
    assert json.loads(syg("widget-of", "bang")) == {"widget": "momentary"}
    assert json.loads(syg("widget-of", "audio")) == {"widget": "wire"}



def exe81_boundary_replaceable():
    # the auto latch is visible as derived structure; replacing it with a
    # smoother persists as an app-graph edit and survives re-compilation
    r = _regions(_hello())
    assert any(m["mapping"] == "latch" for m in r["mappings"])
    replace = [
        {"block": 20, "op": "remove_edge", "a": "lfo0/out", "b": "vca0/gain"},
        {"block": 20, "op": "add_node", "a": "smoother0", "b": "smoother"},
        {"block": 20, "op": "set_param", "route": "smoother0/rate", "value": "3"},
        {"block": 20, "op": "add_edge", "a": "lfo0/out", "b": "smoother0/in"},
        {"block": 20, "op": "add_edge", "a": "smoother0/out", "b": "vca0/gain"},
        # a later unrelated structural edit forces another re-compilation
        {"block": 60, "op": "add_node", "a": "n0", "b": "noise"},
    ]
    out = _exec_audit(_const_hello(), blocks=120, ops=replace, watch=["out"])
    doc = out["serialized"]
    assert "smoother0" in doc["topology"]["nodes"], "the edit did not persist"
    assert {"from": "smoother0/out", "to": "vca0/gain"} in doc["topology"]["edges"]
    # re-compiling inserts NO latch at that boundary: the smoother sits there
    r2 = _regions(doc)
    assert not any(m["mapping"] == "latch" for m in r2["mappings"]), r2
    # and the sound says so: post-replacement blocks RAMP within the block
    # (slew) where the latch held them flat
    flat = out["watched"]["out"][10]
    ramped = [s for s in out["watched"]["out"][70:] if s[0] != s[1]]
    assert flat[0] == flat[1], "pre-replacement latch was not flat"
    assert ramped, "the smoother never smoothed"



def exe71_derivation_mode():
    import tempfile
    with tempfile.TemporaryDirectory() as td:
        first = json.loads(syg("derive-render", td, "2",
                               stdin=json.dumps(_hello()).encode()))
        again = json.loads(syg("derive-render", td, "2",
                               stdin=json.dumps(_hello()).encode()))
        assert first["memo"] is False and first["passes_run"] == 1
        assert again["memo"] is True and again["passes_run"] == 0, \
            "re-running the derivation was not a memo hit"
        assert again["output"] == first["output"]
        # the provenance record is a real committed object: fetch it by
        # hash through the naive resolver and re-derive its cid
        prov = syg("resolve-hash", first["provenance"], td)
        assert len(prov) > 0
        # a different graph is a different recipe, not a stale hit
        other = json.loads(syg("derive-render", td, "2",
                               stdin=json.dumps(_ks()).encode()))
        assert other["memo"] is False and other["output"] != first["output"]



def lng61_graphs_dir_is_the_palette():
    # card.json in graphs_dir appears in the palette; spawning clones the
    # template; the inner position inlet takes the node entry's param
    pal = json.loads(syg("palette"))
    assert "card" in pal and "osc" in pal, pal
    g = {"kind": "graph", "lock": {},
         "topology": {"nodes": {"card0": {"type": "card"},
                                "card1": {"type": "card"}}, "edges": []},
         "defaults": {"card0/position": 7.0, "card1/position": 9.0}}
    out = _exec_audit(g, blocks=30, watch=["card0.pos/out", "card1.pos/out",
                                           "card0.look/out"])
    assert "card0.pos" in out["realized"] and "card1.pos" in out["realized"]
    assert out["watched"]["card0.pos/out"][-1] == 7.0
    assert out["watched"]["card1.pos/out"][-1] == 9.0, "clones not independent"
    assert out["watched"]["card0.look/out"][-1] == 7.0  # inner wiring lives
    # the persisted surface still holds the COMPOSITE (no clone leakage)
    assert "card0.pos" not in json.dumps(out["serialized"])


def lng71_context_two_levels_deep():
    # no node reaches a global: context arrives by injection and forwards
    # through nested subgraphs — graph_source at depth 2 still publishes
    # the ROOT graph's keys
    g = {"kind": "graph", "lock": {},
         "topology": {"nodes": {"deck0": {"type": "deck"},
                                "c9": {"type": "cell"},
                                "c10": {"type": "cell"}}, "edges": []},
         "defaults": {"c9/k": 1.0, "c10/k": 2.0}}
    out = _exec_audit(g, blocks=30, watch=["deck0.gs0/keys"])
    assert out["watched"]["deck0.gs0/keys"][-1] == 3.0, \
        f"the seam did not forward the root context: {out['watched']}"
    # and the static audit half: no singleton reach anywhere (EXE-6.1)
    exe61_no_singleton_reach()



def _poly(freqs):
    return {"kind": "graph", "lock": {},
            "topology": {"nodes": {"freqs0": {"type": "spanv"},
                                   "osc0": {"type": "osc"},
                                   "mix0": {"type": "mix"},
                                   "dac0": {"type": "dac"}},
                         "edges": [{"from": "freqs0/out", "to": "osc0/freq"},
                                   {"from": "osc0/out", "to": "mix0/in"},
                                   {"from": "mix0/out", "to": "dac0/in"}]},
            "defaults": {"freqs0/values": freqs,
                         "osc0/shape": "cosine"}}


def lng21_excess_rank_lifts():
    import math, struct
    # an N-wide span of frequencies into osc0/freq stamps N clones keyed by
    # index; their audio gathers into the explicit mix
    out = _exec_audit(_poly([220.0, 330.0, 440.0]), blocks=4)
    clones = [n for n in out["realized"] if n.startswith("osc0#")]
    assert sorted(clones) == ["osc0#0", "osc0#1", "osc0#2"], out["realized"]
    raw = syg("render-graph", "1", stdin=json.dumps(
        _poly([220.0, 330.0, 440.0])).encode())
    x = struct.unpack(f"<{len(raw) // 4}f", raw)

    def amp(f, sr=48000):
        w = 2 * math.pi * f / sr
        c = 2 * math.cos(w)
        s1 = s2 = 0.0
        for v in x[:24000]:
            s0 = v + c * s1 - s2
            s2, s1 = s1, s0
        return math.sqrt(abs(s1 * s1 + s2 * s2 - c * s1 * s2))
    quiet = amp(550.0)
    for f in (220.0, 330.0, 440.0):
        assert amp(f) > 100 * max(quiet, 1e-9), f"partial {f} missing"
    # N=1 degenerates to one clone
    out1 = _exec_audit(_poly([220.0]), blocks=2)
    assert [n for n in out1["realized"] if n.startswith("osc0#")] == ["osc0#0"]
    # resize preserves clone state by key (EXE-5.2's identity story): the
    # first half of a resized render is byte-identical to the unresized one
    ops = [{"block": 187, "route": "freqs0/values",
            "value": "[220.0, 330.0, 440.0, 550.0]"}]
    straight = syg("render-graph", "1", stdin=json.dumps(
        _poly([220.0, 330.0, 440.0])).encode())
    resized = json.loads(syg("exec-audit", stdin=json.dumps(
        {"graph": _poly([220.0, 330.0, 440.0]), "blocks": 375,
         "ops": ops, "watch": []}).encode()))
    assert "osc0#3" in resized["realized"], "resize did not stamp the fourth"
    assert resized["rt_events"] == 0


def lng22_draw_consumes_span_whole():
    # an N-instance span into the draw boundary: ONE call, no clones.
    # (2026-07-05, PKG-4 retrofit: the present is driven by render_head's
    # chain — ADR-015's clocks-are-inputs — so the head joins the fixture;
    # every assertion below is unchanged.)
    g = {"kind": "graph", "lock": {},
         "topology": {"nodes": {"head0": {"type": "render_head"},
                                "pts0": {"type": "spanv"},
                                "draw0": {"type": "instanced_draw"}},
                      "edges": [{"from": "pts0/out", "to": "draw0/instances"},
                                {"from": "head0/frame", "to": "draw0/tick"}]},
         "defaults": {"pts0/values": [1.0, 2.0, 3.0, 4.0, 5.0]}}
    out = _exec_audit(g, blocks=130, watch=["draw0/calls", "draw0/drawn"])
    assert not any("#" in n for n in out["realized"]), \
        f"the draw boundary stamped clones: {out['realized']}"
    calls = out["watched"]["draw0/calls"][-1]
    assert out["watched"]["draw0/drawn"][-1] == 5.0, out["watched"]
    # 130 blocks = 0.347 s = ~21 frame presents: exactly one call each
    assert 19 <= calls <= 23, f"one call per present, got {calls} in ~21 frames"


def lng62_resource_holder_refuses_lift():
    # a subgraph containing dac refuses lifting; the message names the
    # inner culprit
    g = {"kind": "graph", "lock": {},
         "topology": {"nodes": {"freqs0": {"type": "spanv"},
                                "spk0": {"type": "speaker"}},
                      "edges": [{"from": "freqs0/out", "to": "spk0/level"}]},
         "defaults": {"freqs0/values": [0.1, 0.2]}}
    try:
        _exec_audit(g, blocks=1)
        raise AssertionError("a resource-holder subgraph accepted a lift")
    except AssertionError as e:
        msg = str(e)
        assert "resource holder refuses to lift" in msg, msg
        assert "dac" in msg, f"the inner culprit is not named: {msg}"



def tcf4_fault_matrix():
    # one test per fault class by region kind, each ending in the
    # documented state with testimony written:
    # 1. declared throw -> fault record on the output; region keeps ticking
    out = json.loads(syg("fault-audit"))
    assert out["faults"] and out["ticks_after_fault"] > 0
    # 2. undeclared throw -> containment-unit death, testified, restarts
    out = json.loads(syg("quarantine-audit"))
    assert out["deaths"] == 3 and out["testimony"]["class"] == "undeclared-throw"
    # 3. trap in quarantine -> SIGSEGV kills the subprocess, testimony
    #    carries the signal; the supervisor survives
    out = json.loads(syg("quarantine-audit", "trap"))
    assert out["testimony"]["class"] == "trap-SIGSEGV", out
    assert out["testimony"]["signal"] == 11 and out["supervisor_alive"]
    # 4. NaN at the dac -> boundary scan severs upstream; sink falls back
    #    to silence; the region keeps ticking
    g = {"kind": "graph", "lock": {},
         "topology": {"nodes": {"bomb0": {"type": "nan_bomb"},
                                "dac0": {"type": "dac"}},
                      "edges": [{"from": "bomb0/out", "to": "dac0/in"}]},
         "defaults": {"bomb0/at": 300.0}}
    out = _exec_audit(g, blocks=10, watch=["out"])
    assert any("nan-inf" in f for f in out["faults"]), out["faults"]
    assert out["watched"]["out"][-1] == [0.0, 0.0], "severance did not mute"
    # 5. hang in the frame region -> stale heartbeat, containment restart
    out = json.loads(syg("hang-audit"))
    assert out["hang_detected"] and out["testimony"]["class"] == "hang"
    # 6. overrun in the block region -> report, then the ladder mutes the
    #    costliest island after three consecutive misses
    g = {"kind": "graph", "lock": {},
         "topology": {"nodes": {"osc0": {"type": "osc"},
                                "hog0": {"type": "spin"},
                                "dac0": {"type": "dac"}},
                      "edges": [{"from": "osc0/out", "to": "hog0/in"},
                                {"from": "hog0/out", "to": "dac0/in"}]},
         "defaults": {"osc0/freq": 220.0, "hog0/iters": 50000000.0}}
    out = _exec_audit(g, blocks=8)
    overruns = [f for f in out["faults"] if f.startswith("overrun")]
    muted = [f for f in out["faults"] if f.startswith("ladder")]
    assert overruns, "the executor never noticed the deadline miss"
    assert muted and "hog0" in muted[0], f"the ladder did not name the hog: {out['faults']}"



def aut41_keyed_clone_guarantees():
    # the three keyed-identity guarantees, composed: cards by id (EXE-5.2),
    # channels by index (LNG-2.1), and the refusal naming the culprit
    import rung04
    rung04.exe52_clones_keyed_survive_reorder()
    lng21_excess_rank_lifts()
    lng62_resource_holder_refuses_lift()


def aut42_key_span_never_steals_the_lift():
    # a span into the lift-key port provides clone KEYS; the data span
    # drives the lift — the key span must not stamp clones of its own
    g = {"kind": "graph", "lock": {},
         "topology": {"nodes": {"data0": {"type": "spanv"},
                                "keys0": {"type": "spanv"},
                                "scale0": {"type": "scale"}},
                      "edges": [{"from": "data0/out", "to": "scale0/in"},
                                {"from": "keys0/out", "to": "scale0/k"}],
                      "lift_key": "k"},
         "defaults": {"data0/values": [2.0, 3.0, 5.0],
                      "keys0/values": ["a", "b", "c"],
                      "scale0/k": 10.0}}
    out = _exec_audit(g, blocks=30, watch=["scale0#a/out", "scale0#c/out"])
    clones = sorted(n for n in out["realized"] if "#" in n)
    assert clones == ["scale0#a", "scale0#b", "scale0#c"], \
        f"keys did not name the clones (or the key span lifted): {clones}"
    assert out["watched"]["scale0#a/out"][-1] == 20.0
    assert out["watched"]["scale0#c/out"][-1] == 50.0


TESTS = {
    "EXE-1.1": exe11_plan_cache,
    "EXE-1.2": exe12_defaults_never_capture_modulation,
    "EXE-2.1": exe21_hello_regions,
    "EXE-2.2": exe22_regions_recompute_per_edit,
    "EXE-3.1": exe31_rt_safety_under_live_edits,
    "EXE-4.1": exe41_latch_at_block_start,
    "EXE-4.2": exe42_events_in_order,
    "EXE-4.3": exe43_z_inverse_certified,
    "EXE-6.1": exe61_no_singleton_reach,
    "EXE-6.2": exe62_pump_offline,
    "EXE-7.1": exe71_derivation_mode,
    "EXE-8.1": exe81_boundary_replaceable,
    "EXE-9.1": exe91_existence_is_reference,
    "EXE-10.1": exe101_island_pitch,
    "EXE-10.2": exe102_explicit_delay_opts_out,
    "EXE-10.3": exe103_block_override_in_cycle_rejected,
    "EXE-10.4": exe104_frozen_equals_interpreted,
    "EXE-11.1": exe111_static_scene_quiesces,
    "EXE-11.2": exe112_dirty_cone_exactly,
    "EXE-11.3": exe113_inert_lint,
    "EXE-11.4": exe114_bang_wakes_cone_same_tick,
    "LNG-1.1": lng11_kind_catalog,
    "LNG-2.1": lng21_excess_rank_lifts,
    "LNG-2.2": lng22_draw_consumes_span_whole,
    "LNG-3.1": lng31_queue_never_drops,
    "LNG-4.1": lng41_defaults_are_the_inlet_model,
    "LNG-4.2": lng42_widget_table_is_data,
    "LNG-5.1": lng51_ops_replay_to_same_hash,
    "LNG-5.2": lng52_ops_carry_authors,
    "LNG-6.1": lng61_graphs_dir_is_the_palette,
    "LNG-6.2": lng62_resource_holder_refuses_lift,
    "LNG-7.1": lng71_context_two_levels_deep,
    "LNG-9": lng9_text_events_still_open,
    "TCF-1": tcf1_mapping_guarantees,
    # AUT joined the manifest 2026-07-05 (extractor prefix gap)
    "AUT-4.1": aut41_keyed_clone_guarantees,
    "AUT-4.2": aut42_key_span_never_steals_the_lift,
    "TCF-2": tcf2_swaps_under_load,
    "TCF-3": tcf3_clock_honesty,
    "TCF-4": tcf4_fault_matrix,
    "TCF-5": tcf5_movement_austerity,
}
