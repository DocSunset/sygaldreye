# Pin clang-format version in the dev env

`sh/format.sh` runs clang-format through the nix shell, but the **system**
clang-format on PATH is 22.1.3, which is newer than the nix-pinned version
the repo was canonically formatted with. Running clang-format directly
(outside `sh/format.sh`) reflows the whole tree (~257 .cpp/.hpp files,
+5.7k/-3.6k, pure formatting: column-alignment collapse + line-rejoin) and
diverges from canonical.

This bit the lifting/editor arc: a sub-agent ran the system clang-format
and left a 257-file uncommitted reflow. Caught and stashed
(`stash@{0}` on `lifting-editor-drawfn`, msg "system-clang-format-22.1.3
whole-tree reflow churn") — drop it.

Fix: pin clang-format to the nix-provided version everywhere (so the
system binary can't be used by accident) — e.g. wrap/alias clang-format to
the nix one, or have `sh/format.sh` invoke the nix store path explicitly,
and document that direct `clang-format -i` must not be used. Also exclude
`companion/.venv-melotts/**` (vendored Python venv with .cpp files) from
`format.sh`'s `find` — it errors with "overlapping replacement".
