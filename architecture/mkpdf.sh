#!/usr/bin/env bash
# Generate a single PDF of the architecture book.
# Usage: architecture/mkpdf.sh [output.pdf]
# Deps (fetched via nix if absent): pandoc, typst.
set -euo pipefail
self="$(realpath "$0")"
cd "$(dirname "$self")"

out="${1:-sygaldreye-architecture.pdf}"

if ! command -v pandoc >/dev/null || ! command -v typst >/dev/null; then
  exec nix shell nixpkgs#pandoc nixpkgs#typst --command "$self" "$out"
fi

# typst under nix sees no system fonts; point it at dejavu from the store
if [ -z "${TYPST_FONT_PATHS:-}" ]; then
  TYPST_FONT_PATHS="$(nix build --no-link --print-out-paths nixpkgs#dejavu_fonts)/share/fonts"
  export TYPST_FONT_PATHS
fi

combined="$(mktemp --suffix=.md)"
trap 'rm -f "$combined"' EXIT

# README (how to read) first, then chapters 00..17 and the appendix in
# filename order.
for f in README.md [0-9][0-9]-*.md; do
  cat "$f" >>"$combined"
  printf '\n\n```{=typst}\n#pagebreak()\n```\n\n' >>"$combined"
done

pandoc "$combined" \
  --from gfm+raw_attribute \
  --pdf-engine=typst \
  --toc --toc-depth=1 \
  --metadata title="The Sygaldreye Architecture" \
  --metadata author="Travis West · Claude" \
  --metadata date="$(git log -1 --format=%cs -- .)" \
  -V papersize=a4 \
  -V margin-x=2.2cm -V margin-y=2.4cm \
  -V mainfont="DejaVu Serif" -V monofont="DejaVu Sans Mono" \
  -o "$out"

echo "wrote $(realpath "$out")"
