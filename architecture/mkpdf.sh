#!/usr/bin/env bash
# Generate a single PDF of the architecture book.
# Usage: architecture/mkpdf.sh [--audio] [output.pdf]
#   --audio  read-aloud edition: fenced blocks and tables are replaced by a
#            short spoken placeholder, for text-to-speech narration.
# Deps (fetched via nix if absent): pandoc, typst.
set -euo pipefail
self="$(realpath "$0")"
cd "$(dirname "$self")"

audio=0
if [ "${1:-}" = "--audio" ]; then audio=1; shift; fi
if [ "$audio" = 1 ]; then
  out="${1:-sygaldreye-architecture-audio.pdf}"
  title="The Sygaldreye Architecture — Audio Edition"
else
  out="${1:-sygaldreye-architecture.pdf}"
  title="The Sygaldreye Architecture"
fi

if ! command -v pandoc >/dev/null || ! command -v typst >/dev/null; then
  if [ "$audio" = 1 ]; then
    exec nix shell nixpkgs#pandoc nixpkgs#typst --command "$self" --audio "$out"
  else
    exec nix shell nixpkgs#pandoc nixpkgs#typst --command "$self" "$out"
  fi
fi

# typst under nix sees no system fonts; point it at dejavu from the store
if [ -z "${TYPST_FONT_PATHS:-}" ]; then
  TYPST_FONT_PATHS="$(nix build --no-link --print-out-paths nixpkgs#dejavu_fonts)/share/fonts"
  export TYPST_FONT_PATHS
fi

combined="$(mktemp --suffix=.md)"
trap 'rm -f "$combined"' EXIT

# README (how to read) first, then chapters and appendices in filename order.
for f in README.md [0-9][0-9]-*.md; do
  if [ "$audio" = 1 ]; then
    # Strip fenced code blocks and pipe tables; each contiguous stripped
    # region becomes one spoken placeholder sentence.
    python3 - "$f" >>"$combined" <<'PY'
import sys
MARK = "A figure from the print edition is omitted here.\n"
out, fence, skipping = [], False, False
for ln in open(sys.argv[1]):
    stripped = ln.lstrip()
    if stripped.startswith("```"):
        fence = not fence
        if fence and not skipping:
            out.append(MARK); skipping = True
        if not fence: continue
        continue
    if fence:
        continue
    if stripped.startswith("|"):
        if not skipping:
            out.append(MARK); skipping = True
        continue
    skipping = False
    out.append(ln)
sys.stdout.write("".join(out))
PY
  else
    cat "$f" >>"$combined"
  fi
  printf '\n\n```{=typst}\n#pagebreak()\n```\n\n' >>"$combined"
done

pandoc "$combined" \
  --from gfm+raw_attribute \
  --pdf-engine=typst \
  --toc --toc-depth=1 \
  --metadata title="$title" \
  --metadata author="Travis West · Claude" \
  --metadata date="$(git log -1 --format=%cs -- .)" \
  -V papersize=a4 \
  -V margin-x=2.2cm -V margin-y=2.4cm \
  -V mainfont="DejaVu Serif" -V monofont="DejaVu Sans Mono" \
  -o "$out"

echo "wrote $(realpath "$out")"
