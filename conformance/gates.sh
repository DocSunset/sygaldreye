#!/usr/bin/env bash
# Law gates: mechanical guard rails that pass on an empty repo and constrain
# forever. Run in CI before the conformance suite. Greenfield code is expected
# under src/ (probe/ is exempt — it is a sealed reference).
set -euo pipefail
cd "$(dirname "$0")/.."
fail=0

say() { echo "GATE $1: $2"; }

if [ -d src ]; then
  # SZ-1.1 — no platform branches in escapement/crown/organ sources
  if grep -rn "#ifdef\|#if defined" src/escapement src/crown src/organs 2>/dev/null; then
    say SZ-1.1 "platform conditional in core sources"; fail=1
  fi
  # ABI-1.2 — no hand-written try/catch or serializers in node sources
  if grep -rn "try {\|catch (" src/nodes 2>/dev/null; then
    say ABI-1.2 "hand-written exception handling in node sources"; fail=1
  fi
  if grep -rln "cbor\|encode_" src/nodes 2>/dev/null | grep -v generated; then
    say ABI-1.2 "hand-written serialization in node sources"; fail=1
  fi
  # L9 — no singleton reach from node code
  if grep -rn "::instance()" src/nodes 2>/dev/null; then
    say EXE-6.1 "singleton reach from node code"; fail=1
  fi
  # COR-5.1 — every native declares its clause on line 1 (ADR-033);
  # ADR-034 extends the scan to the executor package (the compile walk)
  find src/nodes src/organs src/executor \( -name '*.cpp' -o -name '*.hpp' \) 2>/dev/null \
    | grep -v generated | while read -r f; do
    head -1 "$f" | grep -qE '^// clause: (machinery|floor|maturity|scaffolding|fixture)' \
      || { say COR-5.1 "native without a clause marker: $f"; exit 1; }
  done || fail=1
  # ADR-034 — a scaffolding marker names the criterion that dissolves it
  if grep -rn "clause: scaffolding" src --include='*.cpp' --include='*.hpp' 2>/dev/null \
       | grep -v "dissolves: [A-Z][A-Z]*-[0-9]"; then
    say ADR-034 "scaffolding without a named dissolution criterion"; fail=1
  fi
fi

# COR-3 — the core-name manifest matches the book (ch. 16 stratum 3)
book_names=$(grep -A40 "## Stratum 3" architecture/16-the-core.md \
  | grep -oP "^\| \K[a-z0-9⁻¹ ·,-]+(?= \|)" | sed 's/[·,]/\n/g' | tr -d '|' \
  | sed 's/^ *//;s/ *$//' | grep -v "^$" | grep -vx "name" | sort -u)
if [ -f conformance/core-names.txt ]; then
  if ! diff <(echo "$book_names") <(sort -u conformance/core-names.txt) >/dev/null; then
    say COR-3 "core-name manifest diverges from ch. 16"; fail=1
  fi
else
  echo "$book_names" > conformance/core-names.txt
  say COR-3 "core-names.txt initialized from the book"
fi

# CNF-1 — manifest freshness: regenerating must be a no-op
python3 conformance/extract_manifest.py 2>/dev/null | diff -q - conformance/manifest.json >/dev/null \
  || { say CNF-1 "manifest.json is stale — regenerate from the book"; fail=1; }

[ "$fail" = 0 ] && echo "all gates green" || exit 1
