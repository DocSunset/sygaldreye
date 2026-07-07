if [[ -z "${IN_NIX_SHELL:-}" ]]; then
    exec nix develop --command bash "$0" "$@"
fi
