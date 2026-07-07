#!/usr/bin/env bash
# Regenerate the DejaVu Sans MSDF atlas and the derived C++ source files.
# Run from the repo root: sh/gen_font_atlas.sh
# Requires: msdf-atlas-gen, python3 (available in the nix devShell).
set -euo pipefail
# shellcheck source=nix_shell_guard.sh
source "$(dirname "$0")/nix_shell_guard.sh"

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FONT_DIR="$REPO_ROOT/assets/fonts"
COMP_DIR="$REPO_ROOT/components/text_mesh"

mkdir -p "$FONT_DIR"

# --- 1. Generate atlas PNG and JSON ---
msdf-atlas-gen \
  -font "$REPO_ROOT/assets/fonts/dejavu_sans_regular.ttf" \
  -chars "[32, 126]" \
  -type msdf \
  -format png \
  -size 32 \
  -pots \
  -imageout "$FONT_DIR/dejavu_sans.png" \
  -json    "$FONT_DIR/dejavu_sans.json"

# --- 2. Regenerate C++ from JSON + PNG ---
python3 "$REPO_ROOT/sh/gen_font_cpp.py" \
  "$FONT_DIR/dejavu_sans.json" \
  "$FONT_DIR/dejavu_sans.png" \
  "$COMP_DIR"

echo "Atlas generation complete."
