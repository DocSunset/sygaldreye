"""Rung 9 — the mesh. Tests written from the criterion text as reached
(BUILDER.md loop). Peers are booted in-process but talk over REAL loopback
sockets under the ADR-035 crypto suite: peer-level conformance (ch. 17).
Never weaken; amend the book by ADR."""
import json
from _helpers import syg


def _mesh(peers, ops):
    payload = {"peers": peers, "ops": ops}
    out = syg("mesh", stdin=json.dumps(payload).encode())
    return json.loads(out)["results"]


def _key(results, i):
    return results[i]["key"]


# ---- MSH-1.1: keypairs, pairing, revocation --------------------------------
def msh11_pairing_revocation_restores():
    # Three peers pair; put a blob on host; the paired quest can fetch it,
    # place on it, and subscribe to it. Revoke the quest's key on host: all
    # three verbs now refuse (severed at the authenticated handshake). Re-pair:
    # every verb works again. An unpaired browser is refused throughout.
    peers = {"host": {}, "quest": {}, "browser": {}}
    ops = [
        {"op": "pair", "a": "host", "b": "quest"},
        {"op": "put", "peer": "host", "bytes": {"/": {"bytes": "aGVsbG8"}}},
        {"op": "advertise", "peer": "host", "run": ["osc"]},
        {"op": "bind", "peer": "host", "ref": "latest", "cid": "CID"},
    ]
    # discover the put cid by running the prefix first
    pre = _mesh(peers, ops[:2])
    cid = pre[1]["cid"]
    ops[3]["cid"] = cid

    def verbs(frm):
        return [
            {"op": "fetch", "from": frm, "to": "host", "cid": cid},
            {"op": "place", "from": frm, "to": "host", "type": "osc"},
            {"op": "subscribe", "from": frm, "to": "host", "ref": "latest"},
        ]

    ops += verbs("quest")           # 4,5,6  — paired: all succeed
    ops += verbs("browser")         # 7,8,9  — never paired: all refused
    ops += [{"op": "revoke", "peer": "host", "of": "quest"}]  # 10
    ops += verbs("quest")           # 11,12,13 — revoked: all refused
    ops += [{"op": "pair", "a": "host", "b": "quest"}]        # 14
    ops += verbs("quest")           # 15,16,17 — re-paired: all succeed
    r = _mesh(peers, ops)

    # paired quest: fetch returns the hash, place lands, subscribe sees the ref
    assert r[4]["ok"] and r[4]["cid"] == cid, r[4]
    assert r[5]["ok"] and r[5]["placed_on"] == "host", r[5]
    assert r[6]["ok"] and r[6]["bound"] == cid, r[6]

    # unpaired browser: every verb refused at the handshake
    for i in (7, 8, 9):
        assert r[i].get("refused") and not r[i].get("ok"), (i, r[i])

    # revoked quest: every verb refused, and specifically at the handshake
    for i in (11, 12, 13):
        assert r[i].get("refused") and r[i].get("reason") == "handshake", (i, r[i])

    # re-paired: restored
    assert r[15]["ok"] and r[15]["cid"] == cid, r[15]
    assert r[16]["ok"] and r[16]["placed_on"] == "host", r[16]
    assert r[17]["ok"] and r[17]["bound"] == cid, r[17]

    # identity is stable + distinct: seeded keys reproduce, peers differ
    k = _mesh({"host": {}, "quest": {}},
              [{"op": "peer-key", "peer": "host"},
               {"op": "peer-key", "peer": "quest"}])
    k2 = _mesh({"host": {}}, [{"op": "peer-key", "peer": "host"}])
    assert k[0]["key"] == k2[0]["key"], "seeded identity must be deterministic"
    assert k[0]["key"] != k[1]["key"], "distinct peers, distinct keys"


# ---- MSH-2.1: unpaired probe refused at every surface ----------------------
def msh21_unpaired_probe_refused():
    # A port scan + protocol probe from an unpaired scanner. Every peer's one
    # listening surface reads a length-framed hello; a legacy HTTP verb or
    # random bytes fail that frame and the socket closes with NO reply. The
    # historical unauthenticated HTTP behavior is demonstrably gone.
    peers = {"host": {}, "quest": {}, "browser": {}}
    probes = [
        "GET / HTTP/1.1\r\nHost: sygaldreye.local\r\n\r\n",  # legacy HTTP
        "POST /graph HTTP/1.1\r\n\r\n{}",                     # legacy write API
        "\x00\x00\x00\x10garbage-bytes!!!",                  # a framed lie
        "PING",                                              # a bare token
    ]
    ops = []
    for who in ("host", "quest", "browser"):     # scan every peer's surface
        for p in probes:
            ops.append({"op": "probe", "to": who, "payload": p})
    # and prove the surface is not merely dead: a paired peer still talks
    ops += [
        {"op": "pair", "a": "host", "b": "quest"},
        {"op": "put", "peer": "host", "bytes": {"/": {"bytes": "aGVsbG8"}}},
    ]
    r = _mesh(peers, ops)
    n = len(probes) * 3
    for i in range(n):
        assert r[i]["refused"], (i, ops[i]["payload"][:8], r[i])
        assert r[i]["bytes_received"] == 0, (i, r[i])  # no plaintext answer
    cid = r[n + 1]["cid"]
    live = _mesh({"host": {}, "quest": {}}, [
        {"op": "pair", "a": "host", "b": "quest"},
        {"op": "put", "peer": "host", "bytes": {"/": {"bytes": "aGVsbG8"}}},
        {"op": "fetch", "from": "quest", "to": "host", "cid": cid},
    ])
    assert live[2]["ok"], live[2]  # the same surface serves an authenticated peer


# ---- MSH-4.1: pull-shaped placement — nothing lands off the run list -------
def msh41_placement_fuzz_no_escape():
    # Fuzz the placement API with arbitrary type names; assert via the peer's
    # registry audit log that ZERO instantiations occur outside its advertised
    # run set. Advertised requests land; everything else is refused.
    import random
    rng = random.Random(90901)
    advertised = ["osc", "gain", "dac"]
    peers = {"host": {}, "agent": {}}
    ops = [
        {"op": "pair", "a": "host", "b": "agent"},
        {"op": "advertise", "peer": "host", "run": advertised},
    ]
    fuzz = []
    for _ in range(300):
        if rng.random() < 0.3:
            t = rng.choice(advertised)
        else:
            t = "".join(rng.choice("abcdefghij_/.$ ") for _ in range(rng.randint(1, 20)))
        fuzz.append(t)
        ops.append({"op": "place", "from": "agent", "to": "host", "type": t})
    ops.append({"op": "audit-log", "peer": "host"})
    r = _mesh(peers, ops)

    placed = r[-1]["instantiated"]
    # every audited instantiation is an advertised type — nothing escaped
    assert set(placed) <= set(advertised), set(placed) - set(advertised)
    # and every per-request result is consistent with the run list
    for i, t in enumerate(fuzz):
        res = r[2 + i]
        if t in advertised:
            assert res.get("ok") and res.get("placed_on") == "host", (t, res)
        else:
            assert not res.get("ok") and res.get("error") == "not-advertised", (t, res)
    # the audit log holds exactly the advertised requests, in order
    expected = [t for t in fuzz if t in advertised]
    assert placed == expected, (placed[:5], expected[:5])


# ---- MSH-3.1: three lists — advertisement enforced, refusal visible --------
def msh31_shell_exec_refused_falls_through():
    # The browser peer doesn't advertise `shell_exec` (selective advertisement
    # IS the sandbox). An engine requesting it there is refused with a TYPED
    # error visible to the requester; placement falls through to a host that
    # does advertise it. With no advertiser among the candidates, the engine
    # reports unplaceable — carrying the refusal trail.
    peers = {"host": {}, "browser": {}, "agent": {}}
    ops = [
        {"op": "pair", "a": "agent", "b": "browser"},
        {"op": "pair", "a": "agent", "b": "host"},
        {"op": "advertise", "peer": "browser", "run": ["osc", "gain"]},
        {"op": "advertise", "peer": "host", "run": ["osc", "shell_exec"]},
        # ask the browser directly: a typed, visible refusal
        {"op": "place", "from": "agent", "to": "browser", "type": "shell_exec"},
        # the engine's placement: browser refuses, falls through to host
        {"op": "place-fallthrough", "from": "agent",
         "candidates": ["browser", "host"], "type": "shell_exec"},
        # no advertiser among the candidates: unplaceable, trail preserved
        {"op": "place-fallthrough", "from": "agent",
         "candidates": ["browser"], "type": "shell_exec"},
    ]
    r = _mesh(peers, ops)

    direct = r[4]
    assert not direct["ok"] and direct["error"] == "not-advertised", direct
    assert direct["type"] == "shell_exec" and direct["peer"] == "browser", direct

    fell = r[5]
    assert fell["ok"] and fell["placed_on"] == "host", fell
    # the browser's refusal is visible in the trail even on success
    assert any(x["peer"] == "browser" and
               x["refusal"]["error"] == "not-advertised"
               for x in fell["refusals"]), fell

    none = r[6]
    assert not none["ok"] and none["unplaceable"], none
    assert none["refusals"][0]["refusal"]["error"] == "not-advertised", none


# ---- MSH-8.1: per-store sharing by key subset ------------------------------
def msh81_second_store_shared_with_subset():
    # A second store graph shared with a SUBSET of paired keys. Both alice and
    # bob are paired with host and can fetch from the common (default) store;
    # only alice is in the private store's share set. Bob's fetch succeeds for
    # the common hash and is refused for the private one — the flat domain is
    # "shared with every paired key" expressed as config, not hardcoded.
    peers = {"host": {}, "alice": {}, "bob": {}}
    prelude = [
        {"op": "pair", "a": "host", "b": "alice"},
        {"op": "pair", "a": "host", "b": "bob"},
        {"op": "put", "peer": "host", "bytes": {"/": {"bytes": "Y29tbW9u"}}},  # "common"
        {"op": "share-store", "peer": "host", "name": "private", "keys": ["alice"]},
    ]
    pre = _mesh(peers, prelude)
    common_cid = pre[2]["cid"]
    store_idx = pre[3]["store"]
    ops = prelude + [
        {"op": "put", "peer": "host", "store": store_idx,
         "bytes": {"/": {"bytes": "c2VjcmV0"}}},  # "secret"
    ]
    pre2 = _mesh(peers, ops)
    private_cid = pre2[4]["cid"]
    ops += [
        {"op": "fetch", "from": "alice", "to": "host", "cid": common_cid},   # 5
        {"op": "fetch", "from": "alice", "to": "host", "cid": private_cid},  # 6
        {"op": "fetch", "from": "bob", "to": "host", "cid": common_cid},     # 7
        {"op": "fetch", "from": "bob", "to": "host", "cid": private_cid},    # 8
    ]
    r = _mesh(peers, ops)
    assert r[5]["ok"] and r[5]["cid"] == common_cid, r[5]
    assert r[6]["ok"] and r[6]["cid"] == private_cid, r[6]   # alice: in the set
    assert r[7]["ok"] and r[7]["cid"] == common_cid, r[7]   # bob: common store
    assert not r[8]["ok"], r[8]                              # bob: excluded hash
    assert r[8]["reason"] == "not-shared-or-absent", r[8]


TESTS = {
    "MSH-1.1": msh11_pairing_revocation_restores,
    "MSH-2.1": msh21_unpaired_probe_refused,
    "MSH-3.1": msh31_shell_exec_refused_falls_through,
    "MSH-4.1": msh41_placement_fuzz_no_escape,
    "MSH-8.1": msh81_second_store_shared_with_subset,
    # 13-native-contract.md: a plugin generated against contract C1 loads on a peer speaking
    "ABI-4.1": None,
    # 14-formats-protocols.md: Wire golden transcripts: a recorded two-peer session (pair,
    "FMT-4": None,
    # 08-mesh-trust.md: ship a graph Quest to host: runs. Ship an unsigned .so: refused,
    "MSH-5.1": None,
    # 08-mesh-trust.md: the browser peer's plugin form is a WASM side module over the
    "MSH-5.2": None,
    # 08-mesh-trust.md: tamper with a take's testimony peer-id; verification fails.
    "MSH-6.1": None,
    # 08-mesh-trust.md: all MSH/STO/PKG integration tests pass with discovery swapped for
    "MSH-7.1": None,
}
