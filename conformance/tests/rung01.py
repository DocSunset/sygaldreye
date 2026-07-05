"""Rung 1 — Formats & naming. FMT-1/FMT-2/NAM-1.1 are REAL tests backed by
the reference oracles in conformance/reference/ (executable specification):
they verify the oracle against fixtures always, then differentially test the
implementation binary (HARNESS.md) — Pending until ./syg exists. Never
weaken a test; amend the book by ADR instead."""
import json, random
from _helpers import Pending, syg, random_value, fixture
from reference import dagcbor, address


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
        got = syg("encode", stdin=json.dumps(v).encode())  # Pending if no ./syg
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


TESTS = {
    "FMT-1": fmt1_encoder_conformance,
    "FMT-2": fmt2_address_roundtrip,
    "NAM-1.1": fmt2_address_roundtrip,  # same property, book cross-reference
    "FMT-5": None,
    "NAM-1.2": None,
    "NAM-2.1": None,
    "NAM-2.2": None,
    "NAM-3.1": None,
    "NAM-4.1": None,
    "NAM-5.1": None,
    "NAM-5.2": None,
    "NAM-5.3": None,
    "NAM-5.4": None,
    "NAM-6.1": None,
    "NAM-6.2": None,
    "NAM-7.1": None,
}
