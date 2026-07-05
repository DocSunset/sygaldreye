"""Rung 3 — The crown. FMT-3 verifies fixture consistency via the reference
tape oracle (runs today), then differentially tests crown replay (Pending
until ./syg exists)."""
import json
from _helpers import Pending, syg, fixture
from reference import tape


def fmt3_tape_topology_equivalence():
    replayed = tape.replay(fixture("hello-cosine.tape"))
    g = fixture("hello-cosine.json")
    assert replayed["nodes"] == g["topology"]["nodes"], "tape/json nodes differ"
    assert replayed["edges"] == g["topology"]["edges"], "tape/json edges differ"
    assert replayed["defaults"] == g["defaults"], "tape/json defaults differ"
    # differential: implementation replay must serialize to the same shape
    got = json.loads(syg("replay-tape",
                         stdin=fixture("hello-cosine.tape").encode()))
    assert got == replayed, "implementation replay differs from oracle"


TESTS = {
    "COR-2": None,
    "FMT-3": fmt3_tape_topology_equivalence,
}
