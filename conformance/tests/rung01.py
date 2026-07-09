"""Rung 1 — Formats & naming. fmt.encoder_conformance/fmt.address_grammar/nam.addresses.parse_print_roundtrip are REAL tests backed by
the reference oracles in conformance/reference/ (executable specification):
they verify the oracle against fixtures always, then differentially test the
implementation binary (HARNESS.md) — Pending until ./syg exists. Never
weaken a test; amend the book by ADR instead."""
import json, random
from _helpers import Pending, syg, random_value, fixture, to_projection
from reference import dagcbor, address, cid


def fmt1_encoder_conformance():
    # oracle vs pinned vectors (always runs)
    vecs = fixture("dag-cbor-vectors.json")["vectors"]
    for v in vecs:
        got = dagcbor.encode(v["value"]).hex()
        assert got == v["hex"], f"oracle mismatch on {v['value']!r}: {got} != {v['hex']}"
    # NaN/Inf rejection
    for bad in (float("nan"), float("inf")):
        try:
            dagcbor.encode(bad); raise AssertionError("NaN/Inf accepted")
        except ValueError:
            pass
    # differential: implementation vs oracle on random values
    rng = random.Random(1729)
    for _ in range(200):
        v = random_value(rng=rng)
        want = dagcbor.encode(v)
        # bytes cannot ride json.dumps raw; the projection carries them (HARNESS.md)
        got = syg("encode", stdin=json.dumps(to_projection(v)).encode())  # Pending if no ./syg
        assert got == want, f"impl != oracle on {v!r}"


def fmt2_address_roundtrip():
    rng = random.Random(42)
    cases = [("refname", "graphs/hello-cosine".replace("/", "%2F"), []),
             ("refname", "graphs%2Fhello", ["nodes", "osc0", "freq"]),
             ("cid", "bafy" + "a" * 20, ["nodes", "we ird:na/me"]),
             ("peerkey", "z6Mk" + "k" * 10, ["stores", "main"])]
    for _ in range(200):
        kind = rng.choice(["refname", "cid", "peerkey"])
        head = ("bafy" + "".join(rng.choice("abcdef234567") for _ in range(20))
                if kind == "cid" else
                "".join(rng.choice("abc:/# %z") for _ in range(1, 9)) or "x")
        steps = ["".join(rng.choice("abc:/# %z09") for _ in range(1, 6))
                 for _ in range(rng.randint(0, 4))]
        cases.append((kind, head, steps))
    for addr in cases:
        printed = address.print_(addr)
        back = address.parse(printed)
        assert back == addr or addr[0] == "refname" and back[1:] == addr[1:],             f"oracle roundtrip failed: {addr} -> {printed!r} -> {back}"
    # differential against the implementation
    for addr in cases[:50]:
        printed = address.print_(addr)
        got = syg("parse-address", stdin=printed.encode())
        want = json.dumps({"kind": addr[0], "head": addr[1], "route": addr[2]},
                          separators=(",", ":")).encode() + b"\n"
        assert got == want, f"impl parse differs on {printed!r}"


# --- the naming session (resolver + kind registry; HARNESS.md `syg naming`) ---

def _hello_objects():
    """The ch. 1/ch. 2 worked-example store fragment: fiat kind nodes, node
    type declarations with (kind, discipline) port promises, and the
    hello-cosine composite. "$name" is the placeholder the session commits
    into a real cid (dependency order)."""
    def porttype(name, ports):
        return {"kind": "node-type", "name": name,
                "ports": {p: {"kind": {"/": f"${k}"}, "discipline": d}
                          for p, (k, d) in ports.items()}}
    return {
        "scalar": {"kind": "kind", "name": "scalar"},
        "audio": {"kind": "kind", "name": "audio"},
        "osc-type": porttype("osc", {"freq": ("scalar", "value"),
                                     "shape": ("scalar", "value"),
                                     "out": ("audio", "block")}),
        "lfo-type": porttype("lfo", {"freq": ("scalar", "value"),
                                     "out": ("scalar", "frame")}),
        "vca-type": porttype("vca", {"in": ("audio", "block"),
                                     "gain": ("scalar", "block"),
                                     "out": ("audio", "block")}),
        "dac-type": porttype("dac", {"in": ("audio", "block")}),
        "lock": {"osc": {"/": "$osc-type"}, "lfo": {"/": "$lfo-type"},
                 "vca": {"/": "$vca-type"}, "dac": {"/": "$dac-type"}},
        "topology": {"nodes": {"osc0": {"type": "osc"}, "lfo0": {"type": "lfo"},
                               "vca0": {"type": "vca"}, "dac0": {"type": "dac"}},
                     "edges": [{"from": "osc0/out", "to": "vca0/in"},
                               {"from": "lfo0/out", "to": "vca0/gain"},
                               {"from": "vca0/out", "to": "dac0/in"}]},
        "defaults": {"osc0/freq": 220.0, "osc0/shape": "cosine",
                     "lfo0/freq": 0.5},
        "hello": {"kind": "graph", "topology": {"/": "$topology"},
                  "defaults": {"/": "$defaults"}, "lock": {"/": "$lock"}},
    }


def _naming(objects, refs, ops):
    out = json.loads(syg("naming", stdin=json.dumps(
        {"objects": objects, "refs": refs, "ops": ops}).encode()))
    return out["cids"], out["results"]


def nam12_location_independence():
    objs = _hello_objects()
    ops = [{"op": "resolve", "addr": "$hello/nodes/osc0/out"}]
    # peer 1 and peer 2 hold the same objects but different environments:
    # different ref names, extra unrelated content on peer 2
    cids1, r1 = _naming(objs, {"graphs/hello-cosine": "$hello"}, ops)
    objs2 = dict(objs, unrelated={"kind": "note", "text": "peer two extra"})
    cids2, r2 = _naming(objs2, {"elsewhere": "$hello"}, ops)
    assert cids1["hello"] == cids2["hello"], "same content, same hash"
    for r in (r1[0], r2[0]):
        assert r["fixity"] == "fixed"
        assert r["value"] == {"kind": {"/": cids1["audio"]}, "discipline": "block"}
    drop_io = lambda r: {k: v for k, v in r.items() if k != "io"}
    assert drop_io(r1[0]) == drop_io(r2[0]), "resolution differs across peers"


def nam21_live_fixed_memo():
    objs = _hello_objects()
    fixed = "$hello/nodes/osc0/freq"
    live = "graphs%2Fhello-cosine:nodes/osc0/freq"  # ref-name slash is escaped
    cids, r = _naming(objs, {"graphs/hello-cosine": "$hello"}, [
        {"op": "resolve", "addr": fixed},      # first: walks containers
        {"op": "resolve", "addr": fixed},      # second: memoized
        {"op": "resolve", "addr": live},
        {"op": "normalize", "addr": live},
    ])
    assert r[0]["fixity"] == "fixed" and r[0]["io"] > 0
    assert r[1]["io"] == 0, f"second resolve performed I/O: {r[1]}"
    assert r[1]["value"] == r[0]["value"]
    assert r[2]["fixity"] == "live"
    want = address.print_(("cid", cids["hello"], ["nodes", "osc0", "freq"]))
    assert r[3]["normalized"] == want, f"{r[3]} != {want}"


def nam22_ref_move_delivers_once():
    objs = _hello_objects()
    objs["defaults2"] = dict(objs["defaults"], **{"osc0/freq": 440.0})
    objs["hello2"] = dict(objs["hello"], defaults={"/": "$defaults2"})
    objs["note"] = {"kind": "note", "text": "unrelated"}
    live = "graphs%2Fhello-cosine:defaults/osc0%2Ffreq"  # defaults keys are routes
    cids, r = _naming(objs, {"graphs/hello-cosine": "$hello", "other": "$note"}, [
        {"op": "subscribe", "addr": live, "as": "A"},
        {"op": "subscribe", "addr": "graphs%2Fhello-cosine:nodes/osc0/out", "as": "B"},
        {"op": "subscribe", "addr": "other:text", "as": "C"},
        {"op": "resolve", "addr": live},
        {"op": "move-ref", "ref": "graphs/hello-cosine", "to": "$hello2"},
        {"op": "events"},
        {"op": "resolve", "addr": live},
        {"op": "move-ref", "ref": "graphs/hello-cosine", "to": "$hello"},
        {"op": "events"},
    ])
    assert r[3]["value"] == 220.0
    first, second = r[5], r[8]
    for burst, now in ((first, cids["hello2"]), (second, cids["hello"])):
        got = sorted((e["to"], e["now"]) for e in burst)
        assert got == [("A", now), ("B", now)], \
            f"expected exactly one delivery each to A and B: {burst}"
    assert r[6]["value"] == 440.0, "subscriber-visible state did not move"


def nam31_routes_survive_insertion():
    objs = _hello_objects()
    # insert nodes/noise0 into the topology (and its vocabulary into the lock)
    objs["noise-type"] = {"kind": "node-type", "name": "noise",
                          "ports": {"out": {"kind": {"/": "$audio"},
                                            "discipline": "block"}}}
    objs["lock2"] = dict(objs["lock"], noise={"/": "$noise-type"})
    topo2 = json.loads(json.dumps(objs["topology"]))
    topo2["nodes"]["noise0"] = {"type": "noise"}
    objs["topology2"] = topo2
    objs["hello2"] = dict(objs["hello"], topology={"/": "$topology2"},
                          lock={"/": "$lock2"})
    _, r = _naming(objs, {}, [
        {"op": "resolve", "addr": "$hello/nodes/osc0/freq"},
        {"op": "resolve", "addr": "$hello2/nodes/osc0/freq"},
    ])
    # the same name-keyed route means the same thing after the insertion
    # (an index-based scheme would have shifted)
    assert r[0]["value"] == r[1]["value"], f"{r[0]} != {r[1]}"
    assert r[0]["value"]["discipline"] == "value"


def nam41_one_kind_system():
    objs = _hello_objects()
    # a committed scalar dataset: same kind node the wire promise links to
    objs["reading"] = {"kind": {"/": "$scalar"}, "value": 21.5}
    cids, r = _naming(objs, {}, [
        {"op": "resolve", "addr": "$reading/kind"},
        {"op": "resolve", "addr": "$hello/nodes/osc0/freq"},
    ])
    dataset_kind = r[0]["value"]
    wire_kind = r[1]["value"]["kind"]
    assert dataset_kind == wire_kind == {"/": cids["scalar"]}, \
        f"kind hashes differ: {dataset_kind} vs {wire_kind}"


def nam71_spans_survive_edits():
    objs = {
        "line1": "the first line\n",
        "line2": "the second line\n",
        "new-top": "a line inserted at the top\n",
        "v1": {"kind": "text", "pieces": [{"/": "$line1"}, {"/": "$line2"}]},
        "v2": {"kind": "text", "pieces": [{"/": "$new-top"}, {"/": "$line1"},
                                          {"/": "$line2"}]},
    }
    pos = len(objs["line1"]) + 4  # "second" inside line2, via v1 positions
    cids, r = _naming(objs, {}, [
        {"op": "span", "of": "$v1", "at": [pos, 6]},
        {"op": "span-text", "of": "$v1",
         "span": {"piece": "$line2", "start": 4, "len": 6}},
        {"op": "span-text", "of": "$v2",
         "span": {"piece": "$line2", "start": 4, "len": 6}},
    ])
    # the minted span is hash-identified, not position-identified
    assert r[0]["span"] == {"piece": cids["line2"], "start": 4, "len": 6}, r[0]
    # same characters in both versions; the position is derived per version
    assert r[1]["text"] == r[2]["text"] == "second"
    assert r[2]["position"] == r[1]["position"] + len(objs["new-top"])
    # structural sharing: both versions answer the span from the SAME piece
    # object (one cid, stored once) — that is what let the link survive


def _legal(frm, to):
    return json.loads(syg("connection-legal", stdin=json.dumps(
        {"from": list(frm), "to": list(to)}).encode()))


def nam51_true_edge():
    assert _legal(("audio", "block"), ("audio", "block")) == \
        {"legal": True, "mapping": None}


def nam52_latch_boundary():
    # the ch. 1 walk: frame-rate LFO out meets the block-rate vca gain
    assert _legal(("scalar", "frame"), ("scalar", "block")) == \
        {"legal": True, "mapping": "latch"}


def nam53_kind_mismatch_illegal():
    # one shared implementation answers every surface (parse-time validation
    # and editor wire-drop both call this same first-order program)
    v = _legal(("draw_call", "frame"), ("audio", "block"))
    assert v["legal"] is False, v


def nam54_no_lookup_during_tick():
    # Structurally guaranteed (cor.escapement_austerity: the escapement links no vocabulary,
    # so it CANNOT reach the registry), and observably audited: tick the
    # frozen hello-cosine movement with the naming instrumentation live and
    # count kind/promise lookups on the tick path. Pending until the
    # escapement exists (rung 2's first deliverable closes this criterion).
    out = json.loads(syg("tick-audit"))
    assert out["ticks"] > 0, out
    assert out["lookups"] == 0, f"kind/promise lookup on the tick path: {out}"


def nam61_rehash_verifies():
    # pinned blake3 vectors (input = repeating 0..250 byte pattern)
    for c in fixture("blake3-vectors.json")["cases"]:
        data = bytes(i % 251 for i in range(c["input_len"]))
        want = cid.text(cid.cid_bytes(cid.RAW, bytes.fromhex(c["hash"])))
        got = syg("hash", stdin=data).decode().strip()
        assert got == want, f"cid mismatch at len {c['input_len']}: {got} != {want}"
        assert json.loads(syg("verify", got, stdin=data)) == {"ok": True}
        if data:  # a corrupted byte is detected
            bad = bytearray(data)
            bad[len(bad) // 2] ^= 1
            assert json.loads(syg("verify", got, stdin=bytes(bad))) == {"ok": False}


def nam62_chunks_dedup():
    rng = random.Random(6262)
    chunk = 256 * 1024
    shared = rng.randbytes(chunk)          # one full pinned-size chunk
    take_a = shared + rng.randbytes(600)   # two takes sharing their first chunk
    take_b = shared + rng.randbytes(4000)

    def put(blobs):
        return json.loads(syg("chunk-put", stdin=json.dumps(
            {"blobs": [to_projection(b) for b in blobs]}).encode()))

    one = put([take_a])
    assert one["objects"] == 3, one        # shared chunk + tail chunk + index
    both = put([take_a, take_b])
    assert both["objects"] == 5, both      # NOT 6 — the shared chunk stored once
    overhead = both["stored_bytes"] - (len(take_a) + len(take_b) - chunk)
    assert 0 < overhead < 500, f"index overhead out of range: {overhead}"
    # determinism: same take, same root; small blob is a single raw object
    assert put([take_a])["roots"] == one["roots"]
    small = put([b"tiny"])
    assert small["objects"] == 1 and small["stored_bytes"] == 4
    assert small["roots"][0] == syg("hash", stdin=b"tiny").decode().strip()


def fmt5_pins_frozen():
    # The ch. 14 pins, frozen 2026-07-05, restated here verbatim. If this test
    # ever disagrees with the implementation, the implementation drifted; if
    # it must change, that is a succession of the spec (ADR-025), never an
    # edit — amend the book first.
    got = json.loads(syg("pins"))
    assert got == {
        "multicodec": {"dag-cbor": 0x71, "raw": 0x55},
        "multihash": {"blake3-256": 0x1E},
        "hash_bytes": 32,
        "multibase": "b",
        "chunk_bytes": 256 * 1024,
        "escape": "%/:# \t\n",
        "tape_records": ["NODE", "LINK", "SET"],
        "edit_ops": ["add_node", "remove_node", "add_edge", "remove_edge",
                     "set_param", "replace_graph"],
        "wire_kinds": {"PAIR": 1, "HELLO": 2, "SUBSCRIBE": 3, "OPS": 4,
                       "FETCH": 5, "QUERY": 6, "PLACE": 7, "FAULT": 8},
    }, f"pin drift: {got}"


TESTS = {
    "fmt.encoder_conformance": fmt1_encoder_conformance,
    "fmt.address_grammar": fmt2_address_roundtrip,
    "nam.addresses.parse_print_roundtrip": fmt2_address_roundtrip,  # same property, book cross-reference
    "fmt.pins_frozen": fmt5_pins_frozen,
    "nam.addresses.location_independent": nam12_location_independence,
    "nam.liveness.live_fixed_memo": nam21_live_fixed_memo,
    "nam.liveness.ref_move_delivers_once": nam22_ref_move_delivers_once,
    "nam.edit_stable_routes.insertion_stable": nam31_routes_survive_insertion,
    "nam.one_kind_system.one_kind_hash": nam41_one_kind_system,
    "nam.promise_oracle.true_edge_legal": nam51_true_edge,
    "nam.promise_oracle.latch_boundary": nam52_latch_boundary,
    "nam.promise_oracle.kind_mismatch_illegal": nam53_kind_mismatch_illegal,
    "nam.promise_oracle.no_lookup_at_tick": nam54_no_lookup_during_tick,
    "nam.hash_format.rehash_verifies": nam61_rehash_verifies,
    "nam.hash_format.chunk_dedup": nam62_chunks_dedup,
    "nam.sequence_traversal.span_survives_edit": nam71_spans_survive_edits,
}
