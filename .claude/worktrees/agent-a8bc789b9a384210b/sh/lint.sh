#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=nix_shell_guard.sh
source "$(dirname "$0")/nix_shell_guard.sh"

# Requires a compilation database; run build.sh first (or any cmake configure).
# Finds the most recently modified compile_commands.json under build/.
DB=$(find build/ -name compile_commands.json | xargs ls -t 2>/dev/null | head -1)
if [[ -z "$DB" ]]; then
    echo "No compile_commands.json found. Run sh/build.sh first." >&2
    exit 1
fi

FILES=$(find . -name '*.cpp' | grep -v build/)

clang-tidy -p "$(dirname "$DB")" $FILES
