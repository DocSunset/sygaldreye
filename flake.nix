{
  description = "sygaldreye greenfield dev environment";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      forAll = f: nixpkgs.lib.genAttrs [ "x86_64-linux" "aarch64-linux" "aarch64-darwin" ]
        (system: f nixpkgs.legacyPackages.${system});
    in {
      devShells = forAll (pkgs: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            gcc cmake ninja pkg-config
            nlohmann_json
            python3
            git
          ];
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
