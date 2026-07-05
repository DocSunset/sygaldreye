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


TESTS = {
    "ABI-1.1": None,
    "ABI-1.2": None,
    "ABI-2.1": None,
    "ABI-2.2": None,
    "ABI-3.1": None,
    "ABI-3.2": None,
    "COR-1": cor1_escapement_austerity,
    "COR-3": None,
    "SZ-1.1": None,
    "SZ-6": None,
    "COR-5.1": None,
    "COR-5.2": None,
}
