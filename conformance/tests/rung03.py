"""Rung 3 — The crown. fmt.boot_tape verifies fixture consistency via the reference
tape oracle (runs today), then differentially tests crown replay (Pending
until ./syg exists)."""
import json, struct
from _helpers import Pending, syg, fixture, golden_audio_check
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


def cor2_ladder_start():
    # cor.crown_sufficiency's ladder start (appendix rung 3): a tape builds hello-cosine AT
    # RUNTIME — escapement + crown + linked natives + tape, zero code paths
    # outside op application — and the running plan renders the golden
    # audio. Deeper: the hand-frozen movement (rung 2) is the FROZEN form of
    # this same graph, so the two renders must be BYTE-IDENTICAL (freezing
    # is a pure optimizer — ADR-013/014). Full liveness (parser, store,
    # mesh instated by tape) closes the rest of cor.crown_sufficiency at rung 4 (sz.boot_without_store).
    live = syg("render-tape", "6", stdin=fixture("hello-cosine.tape").encode())
    golden_audio_check(struct.unpack(f"<{len(live) // 4}f", live))
    frozen = syg("render-movement", "hello-cosine", "6")
    assert live == frozen, \
        "the tape-built graph and the hand-frozen movement diverge"


TESTS = {
    "cor.crown_sufficiency": cor2_ladder_start,
    "fmt.boot_tape": fmt3_tape_topology_equivalence,
}
