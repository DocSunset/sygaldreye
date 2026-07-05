"""Rung 8 — packages & freezer. Tests written from the criterion text as
they are reached (BUILDER.md loop). Never weaken; amend by ADR."""
import json, re
from _helpers import Pending, syg, HERE

ROOT = HERE.parent


def _hello():
    return json.loads((ROOT / "graphs" / "hello-cosine.json").read_text())


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


TESTS = {
    "AUT-2.1": aut21_no_raw_frame_loops,
    "AUT-2.2": None,
    "AUT-5.1": None,
    "FRZ-1.1": None,
    "FRZ-1.2": None,
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
