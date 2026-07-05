"""Rung 8 — packages & freezer. Tests written from the criterion text as
they are reached (BUILDER.md loop). Never weaken; amend by ADR."""
import json, re, subprocess, tempfile
from pathlib import Path
from _helpers import Pending, syg, HERE

ROOT = HERE.parent


def _hello():
    return json.loads((ROOT / "graphs" / "hello-cosine.json").read_text())


def _chime():
    return json.loads((ROOT / "graphs" / "chime.json").read_text())


def _peer(ops):
    return json.loads(syg("peer", stdin=json.dumps({"ops": ops}).encode()))["results"]


_BACKEND_SPLICE = [
    {"op": "add_node", "a": "backend0", "b": "text_cell"},
    {"op": "set_text", "a": "backend0/value", "b": "codegen"},
    {"op": "add_edge", "a": "backend0/out", "b": "realize0/backend"},
]


def _freeze(graph, extra_ops=()):
    # freeze = compile with backend codegen (ADR-014): the backend arrives
    # by WIRING a text cell into realize's published port — ordinary ops
    r = _peer([
        {"op": "set-app", "graph": graph},
        {"op": "open-engine-editor"},
        {"op": "engine-edit", "ops": _BACKEND_SPLICE + list(extra_ops)},
        {"op": "compile"},
        {"op": "app-cid"},
    ])
    return r[3], r[4]["cid"]


def _build_so(source, tag):
    d = Path(tempfile.mkdtemp(prefix=f"syg-frozen-{tag}-"))
    (d / "frozen.cpp").write_text(source)
    so = d / "libfrozen.so"
    cc = subprocess.run(
        ["g++", "-O2", "-fPIC", "-shared", f"-I{ROOT}/src/nodes",
         "-o", str(so), str(d / "frozen.cpp")], capture_output=True)
    assert cc.returncode == 0, cc.stderr.decode()
    return so


def _stat(args, stdin=b""):
    # run syg capturing BOTH streams; stderr carries the block-time stat
    exe = ROOT / "syg"
    r = subprocess.run([str(exe), *args], input=stdin, capture_output=True)
    assert r.returncode == 0, r.stderr.decode()
    return r.stdout, json.loads(r.stderr.splitlines()[-1])["ns_per_block"]


def frz11_ab_chime_interpreted_vs_frozen():
    # A/B chime interpreted-vs-frozen: spectrogram diff == 0 (byte
    # identity, stronger); block-time stat shows the speedup; hot-reload
    # swaps it live (one process, the artifact loads mid-stream)
    c, _ = _freeze(_chime())
    x = c["execution_body"]
    assert x["backend"] == "codegen" and "artifact" in x, x.keys()
    source = _peer([{"op": "set-app", "graph": _chime()},
                    {"op": "open-engine-editor"},
                    {"op": "engine-edit", "ops": _BACKEND_SPLICE},
                    {"op": "compile"},
                    {"op": "cat", "cid": x["artifact"]["/"]}])[4]["bytes"]
    assert "frozen_movement" in source
    so = _build_so(source, "chime")
    interp, interp_ns = _stat(["render-graph", "2"],
                              stdin=json.dumps(_chime()).encode())
    frozen, frozen_ns = _stat(["render-frozen", str(so), "2"])
    import struct
    xs = struct.unpack(f"<{len(interp) // 4}f", interp)
    assert max(abs(x) for x in xs) > 0.5, "the chime never struck"
    assert interp == frozen, \
        "freezing changed the sound, not just the cost (byte diff)"
    assert frozen_ns < interp_ns,         f"no speedup: frozen {frozen_ns} ns/block vs interpreted {interp_ns}"
    # hot-reload: ONE process renders interpreted, loads the .so live at a
    # block boundary, and the artifact takes over the stream (fresh state:
    # the re-strike is the artifact's own t=0; migration joins EXE-5 later)
    swap = syg("render-swap", str(so), "2", stdin=json.dumps(_chime()).encode())
    half = len(interp) // 2
    assert swap[:half] == interp[:half], "the interpreted half diverged"
    assert swap[half:] == frozen[:half], "the artifact did not take over live"


def aut21_no_raw_frame_loops():
    # grep gate for raw frame loops in ugens; exceptions annotated. The
    # stamp generates the rest (leaf_natives.cpp lives in build/generated
    # and never enters this scan).
    hits, annotated = [], 0
    for f in sorted((ROOT / "src" / "nodes").rglob("*.cpp")):
        lines = f.read_text().splitlines()
        for i, ln in enumerate(lines):
            if re.search(r"for \(int i = 0; i < frames", ln):
                window = "\n".join(lines[max(0, i - 3):i + 1])
                if "AUT-2" in window:
                    annotated += 1
                else:
                    hits.append(f"{f.relative_to(ROOT)}:{i + 1}")
    assert not hits, f"unannotated raw frame loops: {hits}"
    assert annotated > 0, "the gate scanned nothing — scope is wrong"


def aut22_stamp_preserves_block_semantics():
    # the kernel-extraction acceptance, kept green forever: a GENERATED
    # stamp (leaf natives) and the hand stamp render BYTE-IDENTICAL audio
    # over the same signal (IEEE-754: x + y == x - (-y) exactly), and the
    # blessed golden still holds after any stamp refactor
    import struct
    from _helpers import golden_audio_check
    def two_osc(mid_nodes, mid_edges):
        return {"kind": "graph", "lock": {},
                "topology": {"nodes": {"osc0": {"type": "osc"},
                                       "osc1": {"type": "osc"},
                                       "dac0": {"type": "dac"},
                                       **mid_nodes},
                             "edges": [{"from": "osc0/out", "to": mid_edges[0]},
                                       {"from": "osc1/out", "to": mid_edges[1]}]
                             + mid_edges[2]},
                "defaults": {"osc0/freq": 220.0, "osc1/freq": 331.0}}
    hand = two_osc({"add0": {"type": "add"}},
                   ["add0/a", "add0/b",
                    [{"from": "add0/out", "to": "dac0/in"}]])
    leaf = two_osc({"neg0": {"type": "neg"}, "sub0": {"type": "sub"}},
                   ["sub0/a", "neg0/a",
                    [{"from": "neg0/out", "to": "sub0/b"},
                     {"from": "sub0/out", "to": "dac0/in"}]])
    a = syg("render-graph", "1", stdin=json.dumps(hand).encode())
    b = syg("render-graph", "1", stdin=json.dumps(leaf).encode())
    assert a == b, "the generated stamp diverged from the hand stamp"
    # and the ugens suite\'s golden acceptance is unchanged
    raw = syg("render-graph", "6", stdin=json.dumps(_hello()).encode())
    golden_audio_check(struct.unpack(f"<{len(raw) // 4}f", raw))


def frz12_unfreeze_is_reading_provenance():
    # unfreeze(artifact) yields the source graph hash; re-freezing is a
    # memo hit — one session, one store, ordinary ops throughout
    r = _peer([
        {"op": "set-app", "graph": _chime()},
        {"op": "open-engine-editor"},
        {"op": "engine-edit", "ops": _BACKEND_SPLICE},
        {"op": "compile"},
        {"op": "app-cid"},
    ])
    c, app_cid = r[3], r[4]["cid"]
    artifact = c["execution_body"]["artifact"]["/"]
    r2 = _peer([
        {"op": "set-app", "graph": _chime()},
        {"op": "open-engine-editor"},
        {"op": "engine-edit", "ops": _BACKEND_SPLICE},
        {"op": "compile"},
        {"op": "unfreeze", "artifact": artifact},
        {"op": "compile"},
        {"op": "app-cid"},
    ])
    assert r2[4]["app"] == r2[6]["cid"] == app_cid, \
        "unfreezing did not walk provenance back to the source graph"
    assert r2[5]["memo"] is True, "re-freezing missed the memo"


TESTS = {
    "AUT-2.1": aut21_no_raw_frame_loops,
    "AUT-2.2": aut22_stamp_preserves_block_semantics,
    "AUT-5.1": None,
    "FRZ-1.1": frz11_ab_chime_interpreted_vs_frozen,
    "FRZ-1.2": frz12_unfreeze_is_reading_provenance,
    "FRZ-2.1": None,
    "FRZ-3.1": None,
    "FRZ-4.1": None,
    "PKG-1.1": None,
    "PKG-2.1": None,
    "PKG-3.1": None,
    "PKG-3.2": None,
    "PKG-4.1": None,
    "PKG-4.2": None,
    "PKG-5.1": None,
    "PKG-6.1": None,
    "PKG-7.1": None,
    "PKG-8.1": None,
}
