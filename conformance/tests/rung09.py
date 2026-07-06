"""Rung 9 — the mesh. Tests written from the criterion text as reached
(BUILDER.md loop). Peers are booted in-process but talk over REAL loopback
sockets under the ADR-035 crypto suite: peer-level conformance (ch. 17).
Never weaken; amend the book by ADR."""
import json
import subprocess
import tempfile
from pathlib import Path
from _helpers import syg, HERE

ROOT = HERE.parent


def _build_plugin_so():
    src = (ROOT / "conformance" / "fixtures" / "plugin_osc.cpp").read_text()
    d = Path(tempfile.mkdtemp(prefix="syg-mesh-plugin-"))
    (d / "p.cpp").write_text(src)
    so = d / "libplugin.so"
    cc = subprocess.run(
        ["g++", "-std=c++20", "-O2", "-fPIC", "-shared",
         f"-I{ROOT}/src/nodes", f"-I{ROOT}/src/crown", f"-I{ROOT}/src/escapement",
         "-o", str(so), str(d / "p.cpp")], capture_output=True)
    assert cc.returncode == 0, cc.stderr.decode()
    return so


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


# ---- MSH-6.1: signed capture testimony -------------------------------------
def msh61_tampered_testimony_fails():
    # A capture's testimony carries the capturing peer's public key and a
    # signature over the take's content hash. Verification is signature-
    # checking: the honest testimony verifies; swapping the peer-id (to
    # frame another peer) fails, and so does swapping the take.
    peers = {"host": {}, "impostor": {}}
    r = _mesh(peers, [
        {"op": "capture", "peer": "host", "bytes": {"/": {"bytes": "dGFrZQ"}}},
        {"op": "peer-key", "peer": "impostor"},
        {"op": "capture", "peer": "impostor", "bytes": {"/": {"bytes": "b3RoZXI"}}},
    ])
    testimony = r[0]["testimony"]
    impostor_key = r[1]["key"]
    other_take = r[2]["take"]

    # honest testimony verifies
    ok = _mesh(peers, [{"op": "verify-capture", "testimony": testimony}])
    assert ok[0]["valid"] is True, ok[0]

    # tamper the peer-id: claim the impostor captured it — signature fails
    framed = dict(testimony, **{"peer-key": impostor_key})
    bad1 = _mesh(peers, [{"op": "verify-capture", "testimony": framed}])
    assert bad1[0]["valid"] is False, bad1[0]

    # tamper the take: same peer, different content — signature fails
    swapped = dict(testimony, take=other_take)
    bad2 = _mesh(peers, [{"op": "verify-capture", "testimony": swapped}])
    assert bad2[0]["valid"] is False, bad2[0]


# ---- ABI-4.1: plugin contract succession (reachability, not equality) ------
def abi41_contract_reachability():
    # A plugin records the contract hash it was built against. It loads on a
    # peer speaking C2 iff C2 declares a migration path from that hash. Same
    # contract loads trivially; a reachable one loads across the chain; an
    # unreachable one is refused with a typed error naming the missing path.
    peers = {"host": {}}
    # host speaks C3, with declared migrations C1 -> C2 -> C3
    setup = [{"op": "set-contract", "peer": "host", "contract": "C3",
              "migrations": [{"from": "C1", "to": "C2"},
                             {"from": "C2", "to": "C3"}]}]
    r = _mesh(peers, setup + [
        {"op": "admit-plugin", "peer": "host", "contract": "C3"},  # identity
        {"op": "admit-plugin", "peer": "host", "contract": "C1"},  # reachable
        {"op": "admit-plugin", "peer": "host", "contract": "C2"},  # reachable
        {"op": "admit-plugin", "peer": "host", "contract": "CX"},  # stranger
    ])
    assert r[1]["loaded"] is True, r[1]
    assert r[2]["loaded"] is True and r[2]["from"] == "C1", r[2]
    assert r[3]["loaded"] is True, r[3]
    ref = r[4]
    assert ref["loaded"] is False and ref["error"] == "no-migration-path", ref
    assert ref["from"] == "CX" and ref["to"] == "C3", ref

    # a one-directional chain: C3 does NOT load on a peer speaking C1
    r2 = _mesh(peers, [
        {"op": "set-contract", "peer": "host", "contract": "C1",
         "migrations": [{"from": "C1", "to": "C2"}, {"from": "C2", "to": "C3"}]},
        {"op": "admit-plugin", "peer": "host", "contract": "C3"},
    ])
    assert r2[1]["loaded"] is False, r2[1]  # succession is directed


# ---- MSH-5.1: graphs flow; plugins gated by signed provenance --------------
def msh51_plugin_trust_gate():
    # A graph dataset ships Quest->host and RUNS (no prompt). An unsigned .so
    # is refused and logged. Signed by an untrusted key: still refused. Signed
    # by a paired key the host's policy trusts: loads hot (real dlopen), the
    # new node type renders, and its provenance is queryable.
    so = str(_build_plugin_so())
    a_graph = {"kind": "graph", "lock": {},
               "topology": {"nodes": {"o": {"type": "osc"}, "d": {"type": "dac"}},
                            "edges": [{"from": "o/out", "to": "d/in"}]},
               "defaults": {"o/freq": 220.0}}
    plugin_graph = {"kind": "graph", "lock": {},
                    "topology": {"nodes": {"o": {"type": "plugin_osc"},
                                           "d": {"type": "dac"}},
                                 "edges": [{"from": "o/out", "to": "d/in"}]},
                    "defaults": {"o/freq": 220.0}}
    peers = {"quest": {}, "host": {}, "stranger": {}}
    r = _mesh(peers, [
        {"op": "pair", "a": "quest", "b": "host"},
        {"op": "set-policy", "peer": "host", "trust": ["quest"]},  # trusts quest
        # a graph flows and realizes without a prompt
        {"op": "ship-graph", "from": "quest", "to": "host", "graph": a_graph},
        # an UNSIGNED plugin: refused, logged
        {"op": "ship-plugin", "from": "quest", "to": "host", "artifact": so},
        # signed by the STRANGER (untrusted): refused
        {"op": "ship-plugin", "from": "stranger", "to": "host", "artifact": so,
         "sign": True},
        # signed by quest (trusted): loads hot
        {"op": "ship-plugin", "from": "quest", "to": "host", "artifact": so,
         "sign": True, "source": "bafyPlugSrc", "toolchain": "gcc-15"},
        # the loaded type renders a real signal
        {"op": "ship-graph", "from": "quest", "to": "host", "graph": plugin_graph},
        {"op": "plugin-provenance", "peer": "host", "type": "plugin_osc"},
    ])
    assert r[2]["ran"] and r[2]["energy"] > 0, r[2]           # graph just runs
    assert r[3]["loaded"] is False and r[3]["error"] == "unsigned", r[3]
    assert r[3]["logged"] is True, r[3]
    assert r[4]["loaded"] is False and r[4]["error"] == "untrusted-signer", r[4]
    assert r[5]["loaded"] is True and r[5]["type"] == "plugin_osc", r[5]
    assert r[6]["ran"] and r[6]["energy"] > 0, r[6]           # the new type sings
    prov = r[7]
    assert prov["known"] and prov["provenance"]["source"] == "bafyPlugSrc", prov
    assert prov["provenance"]["toolchain"] == "gcc-15", prov


# ---- MSH-5.2: the browser's WASM form rides the SAME gate -------------------
def msh52_wasm_side_module_same_gate():
    # The browser peer's plugin form is a WASM side module over the same
    # channel and gate; the policy check is form-agnostic. (Execution is a
    # host concern — ABI-5, rung 10; here the trust decision is exercised.)
    wasm = {"/": {"bytes": "AGFzbQEAAAA"}}  # a wasm magic-header stand-in
    peers = {"agent": {}, "browser": {}, "stranger": {}}
    r = _mesh(peers, [
        {"op": "pair", "a": "agent", "b": "browser"},
        {"op": "set-policy", "peer": "browser", "trust": ["agent"]},
        # unsigned wasm: refused, same error as the native path
        {"op": "ship-plugin", "from": "agent", "to": "browser", "form": "wasm",
         "bytes": wasm, "as": "wasm_osc"},
        # signed by an untrusted signer: refused
        {"op": "ship-plugin", "from": "stranger", "to": "browser", "form": "wasm",
         "bytes": wasm, "sign": True, "as": "wasm_osc"},
        # signed by the trusted agent: accepted (form-agnostic gate)
        {"op": "ship-plugin", "from": "agent", "to": "browser", "form": "wasm",
         "bytes": wasm, "sign": True, "as": "wasm_osc",
         "source": "bafyWasmSrc", "toolchain": "emscripten"},
        {"op": "plugin-provenance", "peer": "browser", "type": "wasm_osc"},
    ])
    assert r[2]["loaded"] is False and r[2]["error"] == "unsigned", r[2]
    assert r[3]["loaded"] is False and r[3]["error"] == "untrusted-signer", r[3]
    assert r[4]["loaded"] is True and r[4]["form"] == "wasm", r[4]
    assert r[4]["executed"] is False, r[4]  # execution lands at rung 10 (ABI-5)
    assert r[5]["known"] and r[5]["provenance"]["form"] == "wasm", r[5]


# ---- FMT-4: wire golden transcripts ----------------------------------------
def fmt4_wire_golden_transcript():
    # A recorded two-peer session (advertise-query, ops, subscribe, place,
    # fetch) replays BYTE-EXACT. The ciphertext isn't reproducible (ephemeral
    # keys, random nonces) but the plaintext message sequence — each a varint
    # wire-kind + canonical dag-cbor body — is, given seeded identities and a
    # fixed script. The peer-level conformance backbone (ch. 17).
    fix = ROOT / "conformance" / "fixtures"
    script = json.loads((fix / "wire-session.json").read_text())
    golden = json.loads((fix / "wire-transcript.golden.json").read_text())

    def transcript():
        out = syg("mesh", stdin=json.dumps(script).encode())
        return json.loads(out)["transcript"]

    live = transcript()
    # 1) replays byte-exact against the committed golden
    assert live == golden, "wire transcript diverged from the golden"
    # 2) deterministic: a second run is byte-identical (replayable)
    assert transcript() == live, "wire transcript is not reproducible"
    # 3) the transcript actually carries the message kinds it claims
    kinds = {m["kind"] for m in live}
    assert {2, 4, 3, 7, 5} <= kinds, kinds  # HELLO, OPS, SUBSCRIBE, PLACE, FETCH
    # 4) bodies are non-empty canonical dag-cbor (hex), not placeholders
    assert all(isinstance(m["body"], str) for m in live)
    assert any(len(m["body"]) > 2 for m in live)


TESTS = {
    "ABI-4.1": abi41_contract_reachability,
    "FMT-4": fmt4_wire_golden_transcript,
    "MSH-5.1": msh51_plugin_trust_gate,
    "MSH-5.2": msh52_wasm_side_module_same_gate,
    "MSH-1.1": msh11_pairing_revocation_restores,
    "MSH-2.1": msh21_unpaired_probe_refused,
    "MSH-3.1": msh31_shell_exec_refused_falls_through,
    "MSH-4.1": msh41_placement_fuzz_no_escape,
    "MSH-6.1": msh61_tampered_testimony_fails,
    "MSH-8.1": msh81_second_store_shared_with_subset,
    # 08-mesh-trust.md: all MSH/STO/PKG integration tests pass with discovery swapped for
    "MSH-7.1": None,
}
