{
  description = "sygaldreye greenfield dev environment";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  # Maturity import (ADR-033): the reference BLAKE3 C implementation, pinned.
  inputs.blake3-src = { url = "github:BLAKE3-team/BLAKE3/1.8.2"; flake = false; };

  outputs = { self, nixpkgs, blake3-src }:
    let
      forAll = f: nixpkgs.lib.genAttrs [ "x86_64-linux" "aarch64-linux" "aarch64-darwin" ]
        (system: f nixpkgs.legacyPackages.${system});
    in {
      devShells = forAll (pkgs: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            gcc cmake ninja pkg-config
            boost nlohmann_json libsodium
            python3
            git
          ];
          BLAKE3_C_DIR = "${blake3-src}/c";
          shellHook = ''
            echo "sygaldreye greenfield shell."
            echo "  python3 conformance/run.py   # the to-do list"
            echo "  ./conformance/gates.sh       # the law"
            echo "  cat BUILDER.md               # the letter"
          '';
        };
      });

      checks = forAll (pkgs: {
        conformance = pkgs.runCommand "conformance" { buildInputs = [ pkgs.python3 ]; } ''
          cd ${self}
          python3 conformance/run.py > $out
        '';
      });
    };
}
