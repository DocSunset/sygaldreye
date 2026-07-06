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


TESTS = {
    "MSH-1.1": msh11_pairing_revocation_restores,
    "MSH-2.1": msh21_unpaired_probe_refused,
    # 13-native-contract.md: a plugin generated against contract C1 loads on a peer speaking
    "ABI-4.1": None,
    # 14-formats-protocols.md: Wire golden transcripts: a recorded two-peer session (pair,
    "FMT-4": None,
    # 08-mesh-trust.md: a request to instantiate `shell_exec` on the browser peer (which
    "MSH-3.1": None,
    # 08-mesh-trust.md: fuzz the placement API with arbitrary type names; zero
    "MSH-4.1": None,
    # 08-mesh-trust.md: ship a graph Quest to host: runs. Ship an unsigned .so: refused,
    "MSH-5.1": None,
    # 08-mesh-trust.md: the browser peer's plugin form is a WASM side module over the
    "MSH-5.2": None,
    # 08-mesh-trust.md: tamper with a take's testimony peer-id; verification fails.
    "MSH-6.1": None,
    # 08-mesh-trust.md: all MSH/STO/PKG integration tests pass with discovery swapped for
    "MSH-7.1": None,
    # 08-mesh-trust.md: configure a second store graph shared with a subset of keys; the
    "MSH-8.1": None,
}
