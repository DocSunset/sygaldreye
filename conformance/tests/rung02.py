"""Rung 2 — the escapement. Tests written from the criterion text as they
are reached (BUILDER.md loop). Never weaken a test; amend the book by ADR."""
import pathlib, shutil, struct, subprocess, tempfile
from _helpers import Pending, syg, golden_audio_check, HERE

SRC = HERE.parent / "src"


def cor1_escapement_austerity():
    # freestanding compile + symbol audit: no vocabulary, no codec, no
    # allocator — an escapement TU may not need a single external symbol
    gxx = shutil.which("g++") or shutil.which("clang++")
    if not gxx:
        raise Pending("no C++ compiler on PATH for the freestanding audit")
    with tempfile.TemporaryDirectory() as td:
        tu = pathlib.Path(td) / "austerity.cpp"
        tu.write_text(
            '#include "escapement.hpp"\n'
            'namespace e = syg::escapement;\n'
            'extern "C" void tick_it(e::movement* m) { e::tick(*m, 128); }\n')
        obj = pathlib.Path(td) / "austerity.o"
        subprocess.run([gxx, "-std=c++20", "-ffreestanding", "-fno-exceptions",
                        "-fno-rtti", "-Wall", "-Werror",
                        "-I", str(SRC / "escapement"),
                        "-c", str(tu), "-o", str(obj)], check=True)
        undefined = subprocess.run(["nm", "-u", str(obj)], capture_output=True,
                                   text=True, check=True).stdout.strip()
        assert undefined == "", f"escapement pulls in symbols: {undefined}"
    # the appendix's rung-2 gate: the hand-frozen hello-cosine movement
    # produces the golden output headless (fixtures/golden-audio.md 1-3;
    # movement-level conformance is born here)
    raw = syg("render-movement", "hello-cosine", "6")
    golden_audio_check(struct.unpack(f"<{len(raw) // 4}f", raw))


def _sources(*dirs, drop_generated=True):
    for d in dirs:
        p = SRC / d
        if not p.exists():
            continue
        for f in list(p.rglob("*.cpp")) + list(p.rglob("*.hpp")):
            if drop_generated and "generated" in f.parts:
                continue
            yield f


def sz11_no_platform_branches():
    for f in _sources("escapement", "crown", "organs", drop_generated=False):
        s = f.read_text()
        assert "#ifdef" not in s and "#if defined" not in s, \
            f"platform conditional in core source {f}"


def abi12_no_handwritten_serializers():
    for f in _sources("nodes"):
        s = f.read_text()
        assert "try {" not in s and "catch (" not in s, \
            f"hand-written exception handling in {f}"
        assert "cbor" not in s and "encode_" not in s, \
            f"hand-written serialization in {f}"


def cor3_core_names_match_book():
    import re
    book = (HERE.parent / "architecture" / "16-the-core.md").read_text()
    stratum = book.split("## Stratum 3")[1].split("## The ratchet")[0]
    names = set()
    for row in re.findall(r"^\| ([^|]+) \|", stratum, re.M):
        if row.strip() == "name":
            continue
        for n in re.split(r"[·,]", row):
            if n.strip():
                names.add(n.strip())
    manifest = set((HERE / "core-names.txt").read_text().split("\n")) - {""}
    assert names == manifest, \
        f"book vs core-names.txt: {sorted(names ^ manifest)}"


CLAUSE = "// clause: (machinery|floor|maturity|scaffolding)"


def cor51_natives_declare_clauses():
    import re
    found = 0
    for f in _sources("nodes", "organs"):
        first = f.read_text().split("\n", 1)[0]
        assert re.match(CLAUSE, first), f"native without a clause marker: {f}"
        found += 1
    assert found > 0, "no natives scanned"
    # and the gate itself enforces it (gates.sh carries the same scan)
    assert "COR-5.1" in (HERE / "gates.sh").read_text()


def cor52_unknown_clause_rejected():
    import re
    # the mechanical half of review rejection: a marker outside the three
    # earned clauses (+ marked scaffolding) does not satisfy the gate regex
    for bogus in ("// clause: because-fast", "// copyright only", ""):
        assert not re.match(CLAUSE, bogus), bogus
    assert re.match(CLAUSE, "// clause: machinery — touches the audio device")


def sz6_trampolines_small():
    tramps = list((SRC / "trampoline").glob("*.cpp"))
    assert tramps, "no trampolines yet"
    for f in tramps:
        code = [ln for ln in f.read_text().split("\n")
                if ln.strip() and not ln.strip().startswith("//")]
        assert len(code) <= 10, f"{f} is {len(code)} lines (SZ-6: at most 10)"


TESTS = {
    "ABI-1.1": None,
    "ABI-1.2": abi12_no_handwritten_serializers,
    "ABI-2.1": None,
    "ABI-2.2": None,
    "ABI-3.1": None,
    "ABI-3.2": None,
    "COR-1": cor1_escapement_austerity,
    "COR-3": cor3_core_names_match_book,
    "SZ-1.1": sz11_no_platform_branches,
    "SZ-6": sz6_trampolines_small,
    "COR-5.1": cor51_natives_declare_clauses,
    "COR-5.2": cor52_unknown_clause_rejected,
}
