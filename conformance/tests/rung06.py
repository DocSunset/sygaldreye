"""Rung 6 — the store. Tests written from the criterion text as they are
reached (BUILDER.md loop). Never weaken a test; amend the book by ADR."""
import base64, json, pathlib
from _helpers import Pending, syg, HERE

ROOT = HERE.parent


def _store(ops, peers=None):
    out = json.loads(syg("store", stdin=json.dumps(
        {"peers": peers or {}, "ops": ops}).encode()))
    return out["results"]


def _hello():
    return json.loads((ROOT / "graphs" / "hello-cosine.json").read_text())


def _proj(b):
    return {"/": {"bytes": base64.b64encode(b).decode().rstrip("=")}}


def sto11_object_machinery():
    r = _store([
        {"op": "put", "bytes": _proj(b"some take bytes")},
        {"op": "get", "cid": "$0"},
        {"op": "get", "cid": "b" + "a" * 30},
    ], peers={"local": {}})
    # ($0 substitution isn't a session feature; re-run with the real cid)
    cid = r[0]["cid"]
    r = _store([
        {"op": "put", "bytes": _proj(b"some take bytes")},
        {"op": "get", "cid": cid},
        {"op": "get", "cid": "b" + "a" * 30},
    ])
    assert r[0]["cid"] == cid, "put is not content-addressed"
    assert r[1] == {"hit": True, "size": 15}
    assert r[2] == {"hit": False}, "unknown hash must be a clean miss"


def sto21_store_graph_face():
    r = _store([
        {"op": "commit-graph", "graph": _hello(), "ref": "graphs/hello-cosine"},
        {"op": "read", "ref": "graphs/hello-cosine", "member": "topology"},
    ])
    topo = r[1]["node"]
    assert set(topo["nodes"]) == {"osc0", "lfo0", "vca0", "dac0"}
    # no ambient store API: node implementations reach no store symbol
    for f in (ROOT / "src" / "nodes").rglob("*.cpp"):
        src = f.read_text()
        assert "store.hpp" not in src and "peer_store" not in src, f
        assert "::instance()" not in src, f


def sto31_compile_is_committed_derivation():
    r = _store([
        {"op": "commit-graph", "graph": _hello(), "ref": "g"},
        {"op": "compile", "ref": "g"},
        {"op": "compile", "ref": "g"},
    ])
    assert r[1]["memo"] is False and r[1]["passes_run"] == 1
    assert r[2]["memo"] is True and r[2]["passes_run"] == 0, \
        "re-compiling ran passes"
    assert r[2]["execution"] == r[1]["execution"]


def sto32_capture_testimony():
    r = _store([
        {"op": "record", "peer": "quest", "graph": _hello(), "seconds": 2},
        {"op": "commit-stream"},
    ], peers={"quest": {}})
    take, testimony_cid = r[0]["take"], r[0]["testimony"]
    assert take.startswith("b") and testimony_cid.startswith("b")
    # the testimony parses and carries peer key + wiring route
    r2 = _store([
        {"op": "record", "peer": "quest", "graph": _hello(), "seconds": 2},
        {"op": "backlinks", "peer": "quest", "cid": take},
    ], peers={"quest": {}})
    assert testimony_cid in r2[1]["backlinks"]
    assert "refused" in r[1] and "recorder" in r[1]["refused"], \
        "committing a stream must be refused at edit time"


def sto41_refs_and_undo():
    r = _store([
        {"op": "commit-graph", "graph": _hello(), "ref": "g"},
        {"op": "edit-commit", "ref": "g", "route": "osc0/freq", "value": 330.0},
        {"op": "trail", "ref": "g"},
        {"op": "undo-ref", "ref": "g"},
        {"op": "trail", "ref": "g"},
    ])
    first, second = r[0]["graph"], r[1]["graph"]
    assert first != second
    assert r[2]["trail"] == [first, second]
    assert r[3]["undone"] and r[3]["now"] == first, "undo must rebind to #a11"
    assert r[4]["trail"] == [first, second, first], "the trail is history"
    # both versions remain fetchable
    r2 = _store([
        {"op": "commit-graph", "graph": _hello(), "ref": "g"},
        {"op": "edit-commit", "ref": "g", "route": "osc0/freq", "value": 330.0},
        {"op": "get", "cid": first},
        {"op": "get", "cid": second},
    ])
    assert r2[2]["hit"] and r2[3]["hit"]


def sto51_provide_compatible():
    hello = _hello()
    r = _store([
        {"op": "commit-graph", "peer": "linux", "graph": hello, "ref": "g"},
    ], peers={"linux": {"provide_all": True}, "quest": {}})
    gcid = r[0]["graph"]
    r = _store([
        {"op": "commit-graph", "peer": "linux", "graph": hello, "ref": "g"},
        {"op": "fetch", "peer": "quest", "from": "linux", "cid": gcid},
        {"op": "has", "peer": "quest", "cid": gcid},
        {"op": "evict", "peer": "quest"},          # cache pressure
        {"op": "has", "peer": "quest", "cid": gcid},
        {"op": "fetch", "peer": "quest", "from": "linux", "cid": gcid},
        {"op": "has", "peer": "quest", "cid": gcid},
    ], peers={"linux": {"provide_all": True}, "quest": {}})
    assert r[2]["has"] is True
    assert r[4]["has"] is False, "unprovided copies must evict"
    assert r[6]["has"] is True, "the provider re-serves after eviction"


def sto52_provisional_until_provided():
    r = _store([
        {"op": "record", "peer": "quest", "graph": _hello(), "seconds": 1},
        {"op": "provided-by", "cid": "$take"},
    ], peers={"quest": {}, "linux": {"provide_all": True}})
    take = r[0]["take"]
    r = _store([
        {"op": "record", "peer": "quest", "graph": _hello(), "seconds": 1},
        {"op": "provided-by", "cid": take},
        {"op": "fetch", "peer": "linux", "from": "quest", "cid": take},
        {"op": "provided-by", "cid": take},
    ], peers={"quest": {}, "linux": {"provide_all": True}})
    assert r[1]["provided_by"] == [], "a fresh capture is provided by nobody"
    assert r[3]["provided_by"] == ["linux"], \
        "the archive's replication must flip the flag (a query, not state)"


def sto61_resumable_fetch():
    size = 1024 * 1024  # 4 chunks + 1 index node = 5 objects
    r = _store([
        {"op": "put-blob", "peer": "linux", "size": size, "provide": True},
    ], peers={"linux": {"provide_all": True}, "quest": {}})
    root = r[0]["cid"]
    r = _store([
        {"op": "put-blob", "peer": "linux", "size": size, "provide": True},
        {"op": "fetch", "peer": "quest", "from": "linux", "cid": root,
         "stop_after": 3},                          # interrupted mid-transfer
        {"op": "fetch", "peer": "quest", "from": "linux", "cid": root},
        {"op": "has", "peer": "quest", "cid": root},
    ], peers={"linux": {"provide_all": True}, "quest": {}})
    assert r[1]["moved"] == 3, r[1]
    assert r[2]["moved"] == 2, \
        f"retry must move only the missing chunks: {r[2]}"
    assert r[3]["has"] is True


def sto71_presets_share_topology():
    hello = _hello()
    preset = json.loads(json.dumps(hello))
    preset["defaults"]["osc0/freq"] = 440.0
    r = _store([
        {"op": "commit-graph", "graph": hello, "ref": "a"},
        {"op": "commit-graph", "graph": preset, "ref": "b"},
    ])
    assert r[0]["topology"] == r[1]["topology"], "one topology object (#b22)"
    assert r[0]["defaults"] != r[1]["defaults"], "two defaults objects"
    assert r[0]["graph"] != r[1]["graph"]


def sto72_orphaned_default_conflicts():
    broken = _hello()
    del broken["topology"]["nodes"]["osc0"]
    broken["topology"]["edges"] = [e for e in broken["topology"]["edges"]
                                   if not e["from"].startswith("osc0")]
    r = _store([{"op": "commit-graph", "graph": broken, "ref": "g"}])
    conflicts = r[0]["conflicts"]
    assert any("osc0/freq" in c for c in conflicts), \
        f"deleting osc0 must surface the orphaned default, not silence: {conflicts}"


def sto81_forgetting_converges():
    r = _store([
        {"op": "record", "peer": "quest", "graph": _hello(), "seconds": 1},
    ], peers={"quest": {}, "linux": {"provide_all": True}, "browser": {}})
    take = r[0]["take"]
    ops = [
        {"op": "record", "peer": "quest", "graph": _hello(), "seconds": 1},
        {"op": "fetch", "peer": "linux", "from": "quest", "cid": take},
        {"op": "fetch", "peer": "browser", "from": "linux", "cid": take},
        # deliberate forgetting: all providers unpin, caches cycle
        {"op": "unprovide-everywhere", "cid": take},
        {"op": "evict", "peer": "quest"},
        {"op": "evict", "peer": "linux"},
        {"op": "evict", "peer": "browser"},
        {"op": "has", "peer": "quest", "cid": take},
        {"op": "has", "peer": "linux", "cid": take},
        {"op": "has", "peer": "browser", "cid": take},
    ]
    r = _store(ops, peers={"quest": {}, "linux": {"provide_all": True},
                           "browser": {}})
    assert not any(r[i]["has"] for i in (7, 8, 9)), "forgetting must converge"


def sto91_backlink_index_is_derived():
    r = _store([
        {"op": "commit-graph", "graph": _hello(), "ref": "g"},
        {"op": "compile", "ref": "g"},
        {"op": "backlinks", "cid": "$g"},
        {"op": "commit-backlink-index"},
        {"op": "commit-backlink-index"},
    ])
    gcid = r[0]["graph"]
    r = _store([
        {"op": "commit-graph", "graph": _hello(), "ref": "g"},
        {"op": "compile", "ref": "g"},
        {"op": "backlinks", "cid": gcid},
        {"op": "commit-backlink-index"},
        {"op": "commit-backlink-index"},
    ])
    assert r[1]["provenance"] in r[2]["backlinks"], \
        "lineage(#a11) must list the execution graph's provenance"
    assert r[3]["index"] == r[4]["index"], \
        "re-deriving the index must be hash-equal"


def _lineage_query(seed_cid):
    return {"kind": "graph", "lock": {},
            "topology": {"nodes": {"s0": {"type": "seed"},
                                   "x0": {"type": "fixpoint"},
                                   "s1": {"type": "seed"},
                                   "f0": {"type": "filter"},
                                   "j0": {"type": "join"}},
                         "edges": [{"from": "s0/out", "to": "x0/in"},
                                   {"from": "s1/out", "to": "f0/in"},
                                   {"from": "x0/out", "to": "j0/a"},
                                   {"from": "f0/out", "to": "j0/b"}]},
            "defaults": {"s0/cids": [seed_cid], "x0/via": "backlinks",
                         "s1/all": True, "f0/key": "kind",
                         "f0/equals": "take", "j0/mode": "and"}}


def lng101_lineage_query():
    setup = [
        {"op": "put-node", "node": {"kind": "node-type", "name": "osc"}},
        {"op": "put-node", "node": {"kind": "node-type", "name": "other"}},
    ]
    r = _store(setup)
    osc_cid, other_cid = r[0]["cid"], r[1]["cid"]
    ops = setup + [
        {"op": "commit-take-chained", "parent": osc_cid, "n": 1},
        {"op": "commit-take-chained", "parent": other_cid, "n": 9},
    ]
    r = _store(ops)
    take1 = r[2]["take"]
    ops = ops + [
        {"op": "commit-take-chained", "parent": take1, "n": 2},
        {"op": "query", "query": _lineage_query(osc_cid)},
        {"op": "query", "query": _lineage_query(osc_cid)},
    ]
    r = _store(ops)
    take2, unrelated = r[4]["take"], r[3]["take"]
    got = set(r[5]["result"])
    assert take1 in got and take2 in got, f"lineage misses a take: {r[5]}"
    assert unrelated not in got, "an unrelated take leaked into the lineage"
    assert r[6]["memo"] is True, "re-running the query must be a memo hit"


def lng102_standing_query_incremental():
    setup = [{"op": "put-node", "node": {"kind": "node-type", "name": "osc"}}]
    r = _store(setup)
    osc_cid = r[0]["cid"]
    ops = setup + [
        {"op": "commit-take-chained", "parent": osc_cid, "n": 1},
        {"op": "standing-query", "query": _lineage_query(osc_cid)},
        {"op": "commit-take-chained", "parent": osc_cid, "n": 2},
    ]
    r = _store(ops)
    full_evals = r[2]["evals"]
    assert r[3]["standing_evals"] < full_evals, \
        "the standing update recomputed more than its dirty cone"
    assert r[3]["standing_size"] > len(r[2]["result"]), \
        "the standing result did not grow within one demand cycle"


def lng103_fixpoint_terminates_on_cycles():
    # a cyclic link structure through refs: A links "r2", B links "r1"
    r = _store([
        {"op": "put-node", "node": {"kind": "note", "next": "r2"}},
        {"op": "put-node", "node": {"kind": "note", "next": "r1"}},
    ])
    a, b = r[0]["cid"], r[1]["cid"]
    q = {"kind": "graph", "lock": {},
         "topology": {"nodes": {"s0": {"type": "seed"},
                                "x0": {"type": "fixpoint"}},
                      "edges": [{"from": "s0/out", "to": "x0/in"}]},
         "defaults": {"s0/cids": [a], "x0/via": "links"}}
    r = _store([
        {"op": "put-node", "node": {"kind": "note", "next": "r2"}},
        {"op": "put-node", "node": {"kind": "note", "next": "r1"}},
        {"op": "bind-ref", "ref": "r1", "cid": a},
        {"op": "bind-ref", "ref": "r2", "cid": b},
        {"op": "query", "query": q},
    ])
    got = set(r[4]["result"])
    assert a in got and b in got, f"the cycle was not traversed: {got}"
    # and, decisively: it returned at all — no query can hang the store


def lng104_no_bespoke_search():
    r = _store([{"op": "palette-query"}, {"op": "backlinks", "cid": "b" + "a" * 30}])
    reg = json.loads((ROOT / "build" / "generated" / "registration.json")
                     .read_text())
    assert set(r[0]["palette"]) == set(reg), r[0]
    assert "evals" in r[1], "backlinks must route through the query evaluator"


TESTS = {
    "STO-1.1": sto11_object_machinery,
    "STO-2.1": sto21_store_graph_face,
    "STO-3.1": sto31_compile_is_committed_derivation,
    "STO-3.2": sto32_capture_testimony,
    "STO-4.1": sto41_refs_and_undo,
    "STO-5.1": sto51_provide_compatible,
    "STO-5.2": sto52_provisional_until_provided,
    "STO-6.1": sto61_resumable_fetch,
    "STO-7.1": sto71_presets_share_topology,
    "STO-7.2": sto72_orphaned_default_conflicts,
    "STO-8.1": sto81_forgetting_converges,
    "STO-9.1": sto91_backlink_index_is_derived,
    "LNG-10.1": lng101_lineage_query,
    "LNG-10.2": lng102_standing_query_incremental,
    "LNG-10.3": lng103_fixpoint_terminates_on_cycles,
    "LNG-10.4": lng104_no_bespoke_search,
}
