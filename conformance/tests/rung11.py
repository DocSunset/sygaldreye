"""Rung 11 — surfaces (the editor, store browser, documents). Tests written
from the criterion text as reached (BUILDER.md loop). The editor is graphs
editing graphs: a live plan whose arbiter takes edit ops exactly as the
gesture nodes emit them. Never weaken; amend the book by ADR."""
import json
from _helpers import syg, HERE

ROOT = HERE.parent


def _hello():
    return json.loads((ROOT / "graphs" / "hello-cosine.json").read_text())


def _edit(ops):
    out = syg("edit", stdin=json.dumps({"ops": ops}).encode())
    return json.loads(out)["results"]


# ---- EDR-2.1: serialize captures the default, never a live sample ----------
def edr21_defaults_not_live_values():
    # vca0/gain is a CONNECTED inlet — modulated by lfo0 (a live cell swinging
    # every block). Editing it updates the fallback DEFAULT; save/reload keeps
    # that default, never a captured LFO sample (EXE-1.2 at the UX level).
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


# ---- EDR-3.1: structural-snapshot undo restores topology + defaults --------
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


TESTS = {
    "EDR-2.1": edr21_defaults_not_live_values,
    "EDR-3.1": edr31_undo_restores_exactly,
    # 09-editor-documents.md: replacing the palette subgraph live (swap+migrate) changes editor
    "EDR-1.1": None,
    # 09-editor-documents.md: scripted gesture test drives the smoother replacement of CMP-4.1
    "EDR-4.1": None,
    # 09-editor-documents.md: walk ground to graphs/hello-cosine to topology to osc0 to type osc  to
    "EDR-5.1": None,
    # 09-editor-documents.md: frontier of a node with 100,000 links paginates without blocking the
    "EDR-5.2": None,
    # 09-editor-documents.md: a document transcluding `graphs/hello-cosine:nodes/osc0/freq`
    "EDR-6.1": None,
    # 09-editor-documents.md: round-trip metric harness: decompose a small permissive C++ file
    "EDR-6.2": None,
    # 09-editor-documents.md: the full editor integration suite runs twice — human-input
    "EDR-7.1": None,
    # 09-editor-documents.md: a probe mapping attached to any edge exposes its value stream
    "EDR-8.1": None,
}
