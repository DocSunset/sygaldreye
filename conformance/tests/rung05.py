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


TESTS = {
    "EXE-1.1": None,
    "EXE-1.2": None,
    "EXE-2.1": exe21_hello_regions,
    "EXE-2.2": exe22_regions_recompute_per_edit,
    "EXE-3.1": None,
    "EXE-4.1": None,
    "EXE-4.2": None,
    "EXE-4.3": None,
    "EXE-6.1": None,
    "EXE-6.2": None,
    "EXE-7.1": None,
    "EXE-8.1": None,
    "EXE-9.1": None,
    "EXE-10.1": None,
    "EXE-10.2": None,
    "EXE-10.3": None,
    "EXE-10.4": None,
    "EXE-11.1": None,
    "EXE-11.2": None,
    "EXE-11.3": None,
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
