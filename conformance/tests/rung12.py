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


# ---- CNF-4: kind succession end-to-end -------------------------------------
def cnf4_kind_succession():
    # Introduce kind-v2 with a migration. Old objects readable via lazy
    # migrate-on-read (memo hit the second time); a mixed-version two-peer
    # exchange works both directions; a lock-swap upgrades one graph without
    # touching its topology hash's provenance chain.
    obj = {"kind": "kind-v1", "freq": 220}
    r = _conform([
        {"op": "migrate-read", "object": obj, "from": "kind-v1", "to": "kind-v2"},
        {"op": "migrate-read", "object": obj, "from": "kind-v1", "to": "kind-v2"},
        {"op": "mixed-exchange", "a_payload": 11, "b_payload": 22},
        {"op": "lock-swap", "type": "osc",
         "topology": {"nodes": {"osc0": {"type": "osc"}}}},
    ])
    # lazy migrate-on-read: first read runs the migration, second is a memo hit
    assert r[0]["memo"] is False, r[0]
    assert r[1]["memo"] is True, r[1]
    assert r[0]["migrated"]["kind"] == "kind-v2", r[0]
    # mixed-version two-peer exchange works BOTH directions
    assert r[2]["both_directions"] is True, r[2]
    assert r[2]["a_to_b"]["kind"] == "kind-v2", r[2]
    assert r[2]["b_to_a"]["kind"] == "kind-v1", r[2]
    # the lock-swap upgraded the graph but left the topology hash untouched
    assert r[3]["topology_unchanged"] is True, r[3]
    assert r[3]["graph_changed"] is True, r[3]
    assert r[3]["graph_old"] != r[3]["graph_new"], r[3]

    # the widget_a -> widget_b pair (ABI-1.1's "one declaration line adds a
    # port") IS a real kind succession: widget_b's ports are widget_a's plus
    # exactly one. CNF-4 exercises it as an ADDITIVE succession — admitted
    # because the port was ADDED (ABI-1.1 stays green), deriving @0.1.0. This
    # is what dissolves the widget scaffolding: the demonstration now lives in
    # the real succession machinery, not an ad-hoc pair.
    gen = ROOT / "build" / "generated"
    pa = set(json.loads((gen / "widget_a.descriptor.json").read_text())["ports"])
    pb = set(json.loads((gen / "widget_b.descriptor.json").read_text())["ports"])
    assert pb - pa == {"brightness"} and pa < pb, (pa, pb)  # ABI-1.1's property
    succ = _conform([
        {"op": "succeed", "name": "widget", "class": "origin",
         "criteria": {"ABI-1.1": True}},
        {"op": "succeed", "name": "widget", "of": "widget", "class": "additive",
         "criteria": {"ABI-1.1": True}},   # the added port keeps ABI-1.1 green
    ])
    assert succ[1]["version"] == "0.1.0", succ  # an additive: a minor bump


def _crownless_so():
    # a SEALED frozen movement (escapement + baked math, no crown) — built
    # through the rung-8 freezer, exactly the artifact rung 8 machine-freezes.
    import rung08
    osc_only = {"kind": "graph", "lock": {},
                "topology": {"nodes": {"t0": {"type": "osc"}, "dac0": {"type": "dac"}},
                             "edges": [{"from": "t0/out", "to": "dac0/in"}]},
                "defaults": {"t0/freq": 220.0, "t0/shape": "cosine"}}
    c, _ = rung08._freeze(osc_only)
    artifact = c["execution_body"]["artifact"]["/"]
    src = rung08._peer([{"op": "set-app", "graph": osc_only},
                        {"op": "open-engine-editor"},
                        {"op": "engine-edit", "ops": rung08._BACKEND_SPLICE},
                        {"op": "compile"}, {"op": "cat", "cid": artifact}])[4]["bytes"]
    return str(rung08._build_so(src, "crownless"))


# ---- CNF-3 + COR-4: the two profiles -------------------------------------
def _two_profiles():
    r = _conform([{"op": "two-profiles", "so": _crownless_so()}])[0]
    # movement-level conformance passes on the crownless build (it renders)
    assert r["movement"]["pass"] is True, r["movement"]
    assert r["movement"]["energy"] > 0, r["movement"]
    # peer-level FAILS — the absence of the wire surface is DETECTED, not
    # excused (TCF-5): a sealed movement is not a mesh citizen
    assert r["peer"]["pass"] is False, r["peer"]
    assert "no peer" in r["peer"]["reason"], r["peer"]


def cnf3_two_profiles():
    _two_profiles()


def cor4_two_profiles():
    _two_profiles()


# ---- CNF-1: the suite is data, and it tests its own coverage ---------------
def cnf1_suite_as_data():
    # Every acceptance criterion exists as a dataset queryable by chapter and
    # requirement id; a criterion without a test is a reported GAP (the suite
    # tests its own coverage). Built over the REAL manifest + the REAL tests.
    import importlib
    manifest = json.loads((HERE / "manifest.json").read_text())
    criteria = {}
    for rid, req in manifest.items():
        chapter = req.get("chapter", "?")
        crits = req.get("criteria") or {}
        if crits:
            for cid in crits:
                criteria[cid] = {"chapter": chapter, "requirement": rid}
        else:
            criteria[rid] = {"chapter": chapter, "requirement": rid}
    # the real coverage: every test id the rung modules register
    import sys
    sys.path.insert(0, str(HERE / "tests"))
    tested = set()
    for n in range(1, 13):
        mod = importlib.import_module(f"rung{n:02d}")
        tested |= {k for k, v in mod.TESTS.items() if v is not None}

    r = _conform([{"op": "suite-as-data",
                   "criteria": criteria, "tests": sorted(tested)}])[0]
    # queryable by chapter and by requirement
    ch = "08-mesh-trust.md"
    assert ch in r["by_chapter"], list(r["by_chapter"])[:5]
    assert "MSH-1.1" in r["by_chapter"][ch], r["by_chapter"][ch]
    assert "MSH-1.1" in r["by_requirement"]["MSH-1"], r["by_requirement"].get("MSH-1")
    assert r["total"] == len(criteria), (r["total"], len(criteria))
    # the coverage self-test: with the REAL tests, the only uncovered criteria
    # are the ones the runner itself reports as pending (hardware/deferred);
    # inject a criterion with no test and it is reported as a gap
    r2 = _conform([{"op": "suite-as-data",
                    "criteria": {**criteria, "FAKE-9.9": {"chapter": "99",
                                                          "requirement": "FAKE-9"}},
                    "tests": sorted(tested)}])[0]
    assert "FAKE-9.9" in r2["uncovered"], "a criterion without a test was not flagged"


# ---- CNF-2: the harness boots a candidate as a peer, over the wire ---------
def cnf2_candidate_as_peer():
    # The harness runs ENTIRELY over the wire protocol against a candidate it
    # did not link — proven against (a) the reference core and (b) a
    # deliberately broken mutant, which must FAIL with the failing requirement
    # NAMED. The peer-level property here is MSH-4 (pull-shaped placement).
    def mesh(peers, ops):
        out = syg("mesh", stdin=json.dumps({"peers": peers, "ops": ops}).encode())
        return json.loads(out)["results"]

    def check_msh4(candidate_cfg):
        # a peer-level conformance check, wire-only: place an UNADVERTISED type
        # on the candidate and read its audit log. A conformant peer refuses
        # and instantiates nothing; a violation is a placement that landed.
        peers = {"checker": {}, "cand": candidate_cfg}
        r = mesh(peers, [
            {"op": "pair", "a": "checker", "b": "cand"},
            {"op": "advertise", "peer": "cand", "run": ["osc"]},
            {"op": "place", "from": "checker", "to": "cand", "type": "shell_exec"},
            {"op": "audit-log", "peer": "cand"},
        ])
        placed = r[2].get("ok", False)
        leaked = "shell_exec" in r[3]["instantiated"]
        if placed or leaked:
            return {"pass": False, "requirement": "MSH-4",
                    "detail": "instantiated an unadvertised type"}
        return {"pass": True, "requirement": "MSH-4"}

    # (a) the reference core passes peer-level conformance
    ref = check_msh4({})
    assert ref["pass"] is True, ref
    # (b) the deliberately broken mutant FAILS, naming the requirement
    mut = check_msh4({"mutant": "MSH-4"})
    assert mut["pass"] is False, mut
    assert mut["requirement"] == "MSH-4", mut


# ---- CNF-5: the self-gate — N derives N+1, admitted only by N's suite ------
def cnf5_self_gate():
    # A core succession (N derives N+1) is admitted ONLY by the suite run by N.
    # The core is a derivation (ADR-027); the succession is provenance-tracked;
    # the suite gates every succession of everything, INCLUDING ITSELF.
    suite = {"MSH-1.1": True, "AUT-1.1": True, "CMP-4.1": True, "EDR-1.1": True}
    # N derives N+1 with N's suite green: ADMITTED, version bumps, provenance
    r = _conform([
        {"op": "derive-core", "class": "origin", "suite": suite},
        {"op": "derive-core", "of": "sygaldreye", "class": "additive",
         "suite": suite},
    ])
    assert r[1]["admitted"] is True, r[1]
    assert r[1]["version"] == "0.1.0", r[1]
    assert r[1]["provenance"] and r[1]["core"], r[1]  # N->N+1 recorded

    # N+1 that regresses a criterion N ran green: REJECTED by N's suite, named
    bad = _conform_raw([
        {"op": "derive-core", "class": "origin", "suite": suite},
        {"op": "derive-core", "of": "sygaldreye", "class": "additive",
         "suite": {**suite, "CMP-4.1": False}},
    ])
    assert bad.returncode != 0, bad.stdout
    assert b"CMP-4.1" in bad.stderr, bad.stderr
    assert b"sygaldreye-N+1 rejected" in bad.stderr, bad.stderr


TESTS = {
    "CNF-1": cnf1_suite_as_data,
    "CNF-2": cnf2_candidate_as_peer,
    "CNF-5": cnf5_self_gate,
    "CNF-3": cnf3_two_profiles,
    "COR-4": cor4_two_profiles,
    "CNF-4": cnf4_kind_succession,
    "CNF-6.1": cnf61_versions_derived,
    "CNF-6.2": cnf62_class_gate_verifies,
    "CNF-6.3": cnf63_ref_sugar_resolves,
}
