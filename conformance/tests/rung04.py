"""Rung 4 — liveness organs. Tests written from the criterion text as they
are reached (BUILDER.md loop). Never weaken a test; amend the book by ADR."""
import json, pathlib
from _helpers import Pending, syg, HERE

ROOT = HERE.parent


def lng81_roundtrip():
    corpus = sorted((ROOT / "graphs").glob("*.json"))
    assert corpus, "graphs_dir corpus missing"
    for f in corpus:
        j = json.loads(f.read_text())
        assert "mappings" not in json.dumps(j), f"derived structure persisted in {f}"
        back = json.loads(syg("roundtrip", stdin=f.read_bytes()))
        assert back == j, f"round-trip not identity for {f}"
        # idempotence: serialize of the parsed form is a fixpoint
        assert json.loads(syg("roundtrip", stdin=json.dumps(back).encode())) == back


def sz31_naive_resolver():
    import tempfile
    # the store package does not exist here (stage 1 "wedged") — the naive
    # resolver needs only an object directory and a hash
    graph = json.dumps(json.loads(
        (ROOT / "graphs" / "hello-cosine.json").read_text())).encode()
    canonical = syg("encode", stdin=graph)
    cid = syg("hash", stdin=canonical).decode().strip()
    with tempfile.TemporaryDirectory() as td:
        (pathlib.Path(td) / cid).write_bytes(canonical)
        assert syg("resolve-hash", cid, td) == canonical
        # a corrupted object never leaves the resolver
        bad = bytearray(canonical)
        bad[len(bad) // 2] ^= 1
        (pathlib.Path(td) / cid).write_bytes(bytes(bad))
        try:
            syg("resolve-hash", cid, td)
            raise AssertionError("corrupt object was served")
        except AssertionError as e:
            assert "verification failed" in str(e), e
        try:
            syg("resolve-hash", "b" + "a" * 20, td)
            raise AssertionError("miss did not error")
        except AssertionError as e:
            assert "object miss" in str(e), e


TESTS = {
    "EXE-5.1": None,
    "EXE-5.2": None,
    "LNG-8.1": lng81_roundtrip,
    "SZ-2.1": None,
    "SZ-3.1": sz31_naive_resolver,
    "SZ-4.1": None,
    "SZ-5.1": None,
    "SZ-7": None,
    "SZ-8": None,
}
