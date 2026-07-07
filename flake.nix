{
  description = "sygaldreye — greenfield dev environment";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  # Maturity import (ADR-033): the reference BLAKE3 C implementation, pinned.
  # Available to rung-1 formats work as $BLAKE3_C_DIR; not yet wired into CMake.
  inputs.blake3-src = { url = "github:BLAKE3-team/BLAKE3/1.8.2"; flake = false; };

  outputs = { self, nixpkgs, blake3-src }:
    let
      forAll = f: nixpkgs.lib.genAttrs [ "x86_64-linux" "aarch64-linux" "aarch64-darwin" ]
        (system: f nixpkgs.legacyPackages.${system});
    in {
      devShells = forAll (pkgs: {
        # Built on gcc16Stdenv (GCC 16.1) so CMake auto-detects a C++26
        # compiler with P2996 static reflection (`^^`, <meta>, std::meta).
        # Reflection needs an explicit flag: -std=c++26 -freflection.
        default = pkgs.mkShell.override { stdenv = pkgs.gcc16Stdenv; } {
          # The lean core (rungs 1–7): formats, crypto, execution. The heavier
          # audio/visual/XR toolchain (glfw, openxr, whisper, sherpa, the NDK)
          # is salvage from probe/probe1's flake — folded back in at rung 8.
          packages = with pkgs; [
            cmake ninja pkg-config
            boost nlohmann_json libsodium
            python3
            git
          ];
          BLAKE3_C_DIR = "${blake3-src}/c";
          shellHook = ''
            echo "sygaldreye greenfield shell."
            echo "  cmake -B build -G Ninja && cmake --build build   # the build"
            echo "  python3 conformance/run.py                       # the to-do list"
            echo "  ./conformance/gates.sh                            # the law"
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
