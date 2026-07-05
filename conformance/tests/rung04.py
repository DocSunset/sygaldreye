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


TESTS = {
    "EXE-5.1": None,
    "EXE-5.2": None,
    "LNG-8.1": lng81_roundtrip,
    "SZ-2.1": None,
    "SZ-3.1": None,
    "SZ-4.1": None,
    "SZ-5.1": None,
    "SZ-7": None,
    "SZ-8": None,
}
