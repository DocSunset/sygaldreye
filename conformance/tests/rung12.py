"""Rung 12 — the self-hosting closure (the suite that IS the system). Tests
written from the criterion text as reached (BUILDER.md loop). Never weaken;
amend the book by ADR."""
import json
import subprocess
from _helpers import syg, HERE

ROOT = HERE.parent


def _conform(ops):
    out = syg("conform", stdin=json.dumps({"ops": ops}).encode())
    return json.loads(out)["results"]


def _conform_raw(ops):
    exe = ROOT / "syg"
    return subprocess.run([str(exe), "conform"],
                          input=json.dumps({"ops": ops}).encode(),
                          capture_output=True)


# ---- CNF-6.1: derived versions over a succession chain ---------------------
def cnf61_versions_derived():
    # A chain (origin, fix, additive, breaking, fix) derives @0.0.1, @0.1.0,
    # @1.0.0, @1.0.1 at the respective heads. Versions are computed from the
    # chain (ADR-032), stored nowhere, so they can never disagree with history.
    green = {"AUT-1.1": True}
    chain = [
        {"op": "succeed", "name": "osc", "class": "origin", "criteria": green},
        {"op": "succeed", "name": "osc", "of": "osc", "class": "fix", "criteria": green},
        {"op": "succeed", "name": "osc", "of": "osc", "class": "additive", "criteria": green},
        {"op": "succeed", "name": "osc", "of": "osc", "class": "breaking", "criteria": green},
        {"op": "succeed", "name": "osc", "of": "osc", "class": "fix", "criteria": green},
    ]
    versions = [x["version"] for x in _conform(chain)]
    assert versions == ["0.0.0", "0.0.1", "0.1.0", "1.0.0", "1.0.1"], versions


# ---- CNF-6.2: an additive that regresses a green predecessor is rejected ----
def cnf62_class_gate_verifies():
    # Classes are VERIFIED, not vowed: an "additive" succession that fails a
    # predecessor criterion is rejected by the gate, with the criterion named.
    ok = _conform([
        {"op": "succeed", "name": "k", "class": "origin",
         "criteria": {"C1": True, "C2": True}},
        {"op": "succeed", "name": "k", "of": "k", "class": "additive",
         "criteria": {"C1": True, "C2": True}},
    ])
    assert ok[1]["version"] == "0.1.0", ok
    bad = _conform_raw([
        {"op": "succeed", "name": "k", "class": "origin",
         "criteria": {"C1": True, "C2": True}},
        {"op": "succeed", "name": "k", "of": "k", "class": "additive",
         "criteria": {"C1": True, "C2": False}},
    ])
    assert bad.returncode != 0, bad.stdout
    assert b"C2" in bad.stderr, bad.stderr
    # a BREAKING succession MAY regress it — that is what breaking means
    br = _conform([
        {"op": "succeed", "name": "k", "class": "origin",
         "criteria": {"C1": True, "C2": True}},
        {"op": "succeed", "name": "k", "of": "k", "class": "breaking",
         "criteria": {"C1": True, "C2": False}},
    ])
    assert br[1]["version"] == "1.0.0", br


# ---- CNF-6.3: ref-sugar name@M.m.p resolves to the chain-walk hash ----------
def cnf63_ref_sugar_resolves():
    green = {"AUT-1.1": True}
    chain = [
        {"op": "succeed", "name": "osc", "class": "origin", "criteria": green},
        {"op": "succeed", "name": "osc", "of": "osc", "class": "fix", "criteria": green},
        {"op": "succeed", "name": "osc", "of": "osc", "class": "additive", "criteria": green},
        {"op": "succeed", "name": "osc", "of": "osc", "class": "breaking", "criteria": green},
        {"op": "succeed", "name": "osc", "of": "osc", "class": "fix", "criteria": green},
    ]
    r = _conform(chain)
    manual_100, manual_101 = r[3]["cid"], r[4]["cid"]
    resolved = _conform(chain + [
        {"op": "resolve", "name": "osc", "at": "1.0.0"},
        {"op": "resolve", "name": "osc", "at": "1.0.1"},
    ])
    assert resolved[5]["cid"] == manual_100, "osc@1.0.0 != chain walk"
    assert resolved[6]["cid"] == manual_101, "osc@1.0.1 != chain walk"


TESTS = {
    "CNF-6.1": cnf61_versions_derived,
    "CNF-6.2": cnf62_class_gate_verifies,
    "CNF-6.3": cnf63_ref_sugar_resolves,
    # 17-conformance-evolution.md: Every acceptance criterion in this book exists
    "CNF-1": None,
    # 17-conformance-evolution.md: The harness runs entirely over the wire
    "CNF-2": None,
    # 17-conformance-evolution.md: The movement profile passes on a crownless build;
    "CNF-3": None,
    # 17-conformance-evolution.md: Introduce kind-v2 with a migration:
    "CNF-4": None,
    # 17-conformance-evolution.md: A core succession (N derives N+1) is admitted only
    "CNF-5": None,
    # 16-the-core.md: Movement-level and peer-level conformance run
    "COR-4": None,
}
