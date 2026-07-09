"""Rung 2 — the escapement. Tests written from the criterion text as they
are reached (BUILDER.md loop). Never weaken a test; amend the book by ADR."""
import json, pathlib, shutil, struct, subprocess, tempfile
from _helpers import Pending, syg, golden_audio_check, HERE

ROOT = HERE.parent
SRC = ROOT / "src"


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


def abi11_one_declaration():
    # widget_b is widget_a plus one field (src/nodes/widgets/widgets.hpp);
    # port, codec, and binding must all surface it with no other edits
    import importlib.util, json
    gen = HERE.parent / "build" / "generated"
    if not gen.exists():
        raise Pending("generated/ not built yet (ninja -C build)")
    da = json.loads((gen / "widget_a.descriptor.json").read_text())["ports"]
    db = json.loads((gen / "widget_b.descriptor.json").read_text())["ports"]
    assert set(db) - set(da) == {"brightness"}, (da, db)
    assert db["brightness"] == {"dir": "in", "kind": "scalar",
                                "discipline": "value"}
    assert {k: v for k, v in db.items() if k != "brightness"} == da, \
        "the one field changed more than itself"
    # the generated codec knows the field exactly where it is declared
    out = json.loads(syg("codec-selftest", "widget_b",
                         stdin=b'{"gain":0.5,"brightness":0.25}'))
    assert out["brightness"] == 0.25 and out["gain"] == 0.5
    try:
        syg("codec-selftest", "widget_a", stdin=b'{"brightness":0.25}')
        raise AssertionError("widget_a's codec accepted an undeclared field")
    except AssertionError as e:
        assert "no in-port" in str(e), e
    # the generated binding (the host-language half) surfaces it too
    spec = importlib.util.spec_from_file_location("bindings", gen / "bindings.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    assert "brightness" in mod.PORTS["widget_b"]
    assert "brightness" not in mod.PORTS["widget_a"]
    b = mod.widget_b.from_projection({"gain": 0.5, "brightness": 0.25})
    assert b.to_projection()["brightness"] == 0.25
    try:
        mod.widget_a.from_projection({"brightness": 0.25})
        raise AssertionError("widget_a's binding accepted an undeclared field")
    except ValueError:
        pass


def abi21_hook_discipline():
    import json
    out = json.loads(syg("hook-audit"))
    assert out["callbacks"] == 10000
    c = out["counts"]
    for rt in ("apply", "process"):  # RT hooks: clean after prepare
        assert c[rt] == {"allocs": 0, "locks": 0}, (rt, c)
    assert c["create"]["allocs"] >= 1, "create may allocate state (and did not?)"


def abi22_create_never_acquires():
    import json
    assert json.loads(syg("create-audit", "good")) == {"ok": True}
    try:
        syg("create-audit", "bad")
        raise AssertionError("create()-time acquisition was not stopped")
    except AssertionError as e:
        assert "allocation-discipline" in str(e), e


def abi31_declared_fault_converts():
    import json
    out = json.loads(syg("fault-audit"))
    assert out["faults"] == [{"route": "nodes/parsey0", "class": "malformed input"}]
    assert out["ticks"] == 10 and out["ticks_after_fault"] > 0, \
        "the region did not keep ticking past the fault"


def abi32_undeclared_throw_kills_subprocess():
    import json
    out = json.loads(syg("quarantine-audit"))
    assert out["supervisor_alive"] is True
    assert out["testimony"]["route"] == "nodes/boom0", out
    assert out["restarts"] == 2, "supervisor's wired restart ladder (intensity 2)"
    assert out["deaths"] == 3, "each restart died again; then severance"


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


CLAUSE = "// clause: (machinery|floor|maturity|scaffolding|fixture)"


def cor51_natives_declare_clauses():
    import re
    found = 0
    for f in _sources("nodes", "organs"):
        first = f.read_text().split("\n", 1)[0]
        assert re.match(CLAUSE, first), f"native without a clause marker: {f}"
        found += 1
    assert found > 0, "no natives scanned"
    # and the gate itself enforces it (gates.sh carries the same scan)
    assert "cor.native_ledger_discipline.clause_marker_required" in (HERE / "gates.sh").read_text()


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
        assert len(code) <= 10, f"{f} is {len(code)} lines (sz.trampolines: at most 10)"



def aut11_kernel_contract():
    # the kernel suite green, exercised directly against the salvage
    out = json.loads(syg("kernel-audit"))
    assert out["suite"] == "green" and out["checks"] > 100000
    # a kernel holding wall-clock state fails review by grep; the metro
    # exception, when metro exists, must be documented in-source
    import re
    banned = re.compile(r"\btime\(|epoch|wall_?clock", re.I)
    for f in (SRC / "nodes" / "synth_core").glob("*.[ch]pp"):
        code = "\n".join(ln.split("//")[0] for ln in f.read_text().split("\n"))
        for m in banned.finditer(code):  # comments may DISCUSS the rule
            assert "metro exception" in f.read_text(), \
                f"wall-clock state in {f.name} without the documented exception"


def aut12_n1_granularity():
    # island readiness: every kernel ticks correctly at N=1 (the audit
    # compares 128 single-sample calls against one 128-frame call, per
    # native, byte-equal) — and the islands themselves prove it end to end
    out = json.loads(syg("kernel-audit"))
    assert out["suite"] == "green"


def aut31_descriptors_generated():
    import json as _json
    gen = ROOT / "build" / "generated"
    if not gen.exists():
        raise Pending("generated/ not built")
    # adding a field surfaced port + promises (abi.one_declaration.field_surfaces_port's pair) ...
    db = _json.loads((gen / "widget_b.descriptor.json").read_text())["ports"]
    assert db["brightness"] == {"dir": "in", "kind": "scalar",
                                "discipline": "value"}
    # ... and its WIDGET, derived from kind + metadata through the data
    # table — no editor or descriptor code changed anywhere
    w = _json.loads(syg("widget-of", db["brightness"]["kind"]))
    assert w["widget"] == "number"
    # zero hand-maintained descriptor tables: every native TU takes its
    # port lists from the generated header
    for f in (SRC / "nodes" / "hello_natives").glob("*_native*.cpp"):
        src = f.read_text()
        if "in_ports" in src or "out_ports" in src:
            assert "syg::generated::" in src, \
                f"{f.name} carries a hand-maintained port table"


TESTS = {
    # AUT joined the manifest 2026-07-05 (ch. 12 had been silently absent —
    # extractor prefix gap). None = pending: write each from criterion text.
    "aut.kernel_contract.no_wallclock_state": aut11_kernel_contract,
    "aut.kernel_contract.ticks_at_single_sample": aut12_n1_granularity,
    "aut.generated_descriptors.field_surfaces_descriptor": aut31_descriptors_generated,
    "abi.one_declaration.field_surfaces_port": abi11_one_declaration,
    "abi.one_declaration.no_handwritten_adapters": abi12_no_handwritten_serializers,
    "abi.hook_discipline.rt_hooks_clean": abi21_hook_discipline,
    "abi.hook_discipline.create_acquires_fails": abi22_create_never_acquires,
    "abi.fault_declaration.declared_fault_recorded": abi31_declared_fault_converts,
    "abi.fault_declaration.undeclared_throw_kills": abi32_undeclared_throw_kills_subprocess,
    "cor.escapement_austerity": cor1_escapement_austerity,
    "cor.ratchet_enforcement": cor3_core_names_match_book,
    "sz.platform_invariance.ci_grep_gate": sz11_no_platform_branches,
    "sz.trampolines": sz6_trampolines_small,
    "cor.native_ledger_discipline.clause_marker_required": cor51_natives_declare_clauses,
    "cor.native_ledger_discipline.no_clause_rejected": cor52_unknown_clause_rejected,
}
