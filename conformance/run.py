#!/usr/bin/env python3
"""The conformance runner. Zero dependencies. This output IS the to-do list.

Usage:  python3 conformance/run.py [--rung N]

Every acceptance criterion in the book appears in manifest.json (regenerate
via extract_manifest.py). Tests live in tests/rung*.py: each module defines
TESTS = {"NAM-1.1": callable_or_None}. None = pending (not yet written).
A callable returns normally (pass) or raises (fail, with the reason).

The runner walks the twelve build rungs of Appendix A in order and reports
THE FIRST UNMET GATE. That gate is the next thing to work on — always.
Never edit a test to make it pass: amend the book via ADR, regenerate the
manifest, and let the test follow the book.
"""
import importlib.util, json, pathlib, re, sys, traceback

HERE = pathlib.Path(__file__).resolve().parent
RUNG_NAMES = {
    1: "Formats & naming", 2: "The escapement", 3: "The crown",
    4: "Liveness organs", 5: "Execution semantics", 6: "The store",
    7: "Compile & realize", 8: "Packages & freezer", 9: "The mesh",
    10: "Hosts (Python, wasm)", 11: "Surfaces", 12: "Self-hosting closure",
}

manifest = json.loads((HERE / "manifest.json").read_text())

sys.path.insert(0, str(HERE / "tests"))
sys.path.insert(0, str(HERE))          # so tests can `from reference import ...`
from _helpers import Pending  # noqa: E402

# Gather registered tests from tests/rung*.py
tests = {}
for f in sorted((HERE / "tests").glob("rung*.py")):
    spec = importlib.util.spec_from_file_location(f.stem, f)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    tests.update(getattr(mod, "TESTS", {}))

only = None
if len(sys.argv) == 3 and sys.argv[1] == "--rung":
    only = int(sys.argv[2])

# Index criteria by rung
by_rung = {}
for rid, req in manifest.items():
    if req["rung"] is None:
        continue
    for acid, text in req["criteria"].items():
        by_rung.setdefault(req["rung"], []).append((acid, text))
    if not req["criteria"]:
        by_rung.setdefault(req["rung"], []).append((rid, req["text"]))

first_unmet = None
passed = set()
total = {"pass": 0, "fail": 0, "pending": 0, "uncovered": 0}
for rung in sorted(by_rung):
    if only and rung != only:
        continue
    results = {"pass": [], "fail": [], "pending": [], "uncovered": []}
    for acid, text in sorted(by_rung[rung]):
        fn = tests.get(acid, "ABSENT")
        if fn == "ABSENT":
            results["uncovered"].append(acid)
        elif fn is None:
            results["pending"].append(acid)
        else:
            try:
                fn()
                results["pass"].append(acid)
                passed.add(acid)
            except Pending as e:
                results["pending"].append(acid)
            except Exception:
                results["fail"].append((acid, traceback.format_exc(limit=2)))
    n = sum(len(v) for v in results.values())
    ok = len(results["pass"])
    gate = "GREEN" if ok == n else "unmet"
    if gate != "GREEN" and first_unmet is None:
        first_unmet = rung
    print(f"rung {rung:2d} — {RUNG_NAMES[rung]:24s} {ok}/{n} {gate}")
    for acid, tb in results["fail"]:
        print(f"    FAIL {acid}\n{tb}")
    for k in total:
        total[k] += len(results[k])

# The dissolution gate (ADR-034): scaffolding names the criterion that
# retires it; the moment that criterion is GREEN the scaffolding must be
# gone. A live marker naming a passing criterion fails the suite.
SRC_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hh", ".hpp", ".inl", ".py"}
for f in sorted(p for p in (HERE.parent / "src").rglob("*")
                if p.suffix in SRC_SUFFIXES):
    for m in re.finditer(r"[Dd]issolve[sd]?:\s*([A-Z]+-[0-9]+(?:\.[0-9]+)*)",
                         f.read_text()):
        if m.group(1) in passed:
            total["fail"] += 1
            first_unmet = first_unmet or min(by_rung)
            print(f"DISSOLUTION GATE: {f.relative_to(HERE.parent)} still "
                  f"carries scaffolding for GREEN {m.group(1)} — the walk "
                  f"outlived its criterion")

print()
print(f"pass {total['pass']}  fail {total['fail']}  pending {total['pending']}"
      f"  uncovered {total['uncovered']}")
if total["uncovered"]:
    print("uncovered criteria are themselves a reported gap (CNF-1).")
if first_unmet:
    print(f"\nYOU ARE HERE: rung {first_unmet} — {RUNG_NAMES[first_unmet]}.")
    print("Next action: take that rung's first pending/failing criterion,")
    print("write or fix exactly enough to turn it green, and re-run.")
else:
    print("\nAll gates green. The suite is the system; the system exists.")
# CI semantics: pending work is not failure; broken or uncovered is.
sys.exit(1 if (total["fail"] or total["uncovered"]) else 0)
