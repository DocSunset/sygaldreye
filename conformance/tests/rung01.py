"""Rung 1 — Formats & naming. FMT-1/FMT-2/NAM-1.1 are REAL tests backed by
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
    "FMT-1": fmt1_encoder_conformance,
    "FMT-2": fmt2_address_roundtrip,
    "NAM-1.1": fmt2_address_roundtrip,  # same property, book cross-reference
    "FMT-5": fmt5_pins_frozen,
    "NAM-1.2": None,
    "NAM-2.1": None,
    "NAM-2.2": None,
    "NAM-3.1": None,
    "NAM-4.1": None,
    "NAM-5.1": None,
    "NAM-5.2": None,
    "NAM-5.3": None,
    "NAM-5.4": None,
    "NAM-6.1": nam61_rehash_verifies,
    "NAM-6.2": nam62_chunks_dedup,
    "NAM-7.1": None,
}
