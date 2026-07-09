#!/usr/bin/env bash
# Law gates: mechanical guard rails that pass on an empty repo and constrain
# forever. Run in CI before the conformance suite. Greenfield code is expected
# under src/ (probe/ is exempt — it is a sealed reference).
set -euo pipefail
cd "$(dirname "$0")/.."
fail=0

say() { echo "GATE $1: $2"; }

if [ -d src ]; then
  # sz.platform_invariance.ci_grep_gate — no platform branches in escapement/crown/organ sources
  if grep -rn "#ifdef\|#if defined" src/escapement src/crown src/organs 2>/dev/null; then
    say sz.platform_invariance.ci_grep_gate "platform conditional in core sources"; fail=1
  fi
  # abi.one_declaration.no_handwritten_adapters — no hand-written try/catch or serializers in node sources
  if grep -rn "try {\|catch (" src/nodes 2>/dev/null; then
    say abi.one_declaration.no_handwritten_adapters "hand-written exception handling in node sources"; fail=1
  fi
  if grep -rln "cbor\|encode_" src/nodes 2>/dev/null | grep -v generated; then
    say abi.one_declaration.no_handwritten_adapters "hand-written serialization in node sources"; fail=1
  fi
  # law.executors_own_pacing — no singleton reach from node code
  if grep -rn "::instance()" src/nodes 2>/dev/null; then
    say exe.executor_contract.no_singleton_reach "singleton reach from node code"; fail=1
  fi
  # cor.native_ledger_discipline.clause_marker_required — every native declares its clause on line 1 (ADR-033);
  # ADR-034 extends the scan to the executor package (the compile walk).
  # (|| true so absent core dirs don't trip pipefail before any native lands.)
  natives=$(find src/nodes src/organs src/executor \( -name '*.cpp' -o -name '*.hpp' \) \
              2>/dev/null | grep -v generated || true)
  for f in $natives; do
    head -1 "$f" | grep -qE '^// clause: (machinery|floor|maturity|scaffolding|fixture)' \
      || { say cor.native_ledger_discipline.clause_marker_required "native without a clause marker: $f"; fail=1; }
  done
  # ADR-034 — a scaffolding marker names the criterion that dissolves it
  if grep -rn "clause: scaffolding" src --include='*.cpp' --include='*.hpp' 2>/dev/null \
       | grep -v "dissolves: [A-Z][A-Z]*-[0-9]"; then
    say ADR-034 "scaffolding without a named dissolution criterion"; fail=1
  fi
fi

# cor.ratchet_enforcement — the core-name manifest matches the book (ch. 16 stratum 3)
book_names=$(grep -A40 "## Stratum 3" architecture/16-the-core.md \
  | grep -oP "^\| \K[a-z0-9⁻¹ ·,-]+(?= \|)" | sed 's/[·,]/\n/g' | tr -d '|' \
  | sed 's/^ *//;s/ *$//' | grep -v "^$" | grep -vx "name" | sort -u)
if [ -f conformance/core-names.txt ]; then
  if ! diff <(echo "$book_names") <(sort -u conformance/core-names.txt) >/dev/null; then
    say cor.ratchet_enforcement "core-name manifest diverges from ch. 16"; fail=1
  fi
else
  echo "$book_names" > conformance/core-names.txt
  say cor.ratchet_enforcement "core-names.txt initialized from the book"
fi

# cnf.suite_as_data — manifest freshness: regenerating must be a no-op
python3 conformance/extract_manifest.py 2>/dev/null | diff -q - conformance/manifest.json >/dev/null \
  || { say cnf.suite_as_data "manifest.json is stale — regenerate from the book"; fail=1; }

[ "$fail" = 0 ] && echo "all gates green" || exit 1
