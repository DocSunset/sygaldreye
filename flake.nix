{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
  inputs.openxr-sdk.url = "github:KhronosGroup/OpenXR-SDK?ref=release-1.1.60";
  inputs.openxr-sdk.flake = false;

  outputs = { self, nixpkgs, openxr-sdk }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
        config.allowUnfree = true;
        config.android_sdk.accept_license = true;
      };

      androidComposition = pkgs.androidenv.composeAndroidPackages {
        platformVersions = [ "34" ];
        buildToolsVersions = [ "34.0.0" ];
        includeNDK = true;
        ndkVersions = [ "27.3.13750724" ];
      };

      ndkRoot = "${androidComposition.androidsdk}/libexec/android-sdk/ndk/27.3.13750724";

      msdfAtlasGen = pkgs.stdenv.mkDerivation {
        name = "msdf-atlas-gen";
        src = pkgs.fetchFromGitHub {
          owner = "Chlumsky";
          repo  = "msdf-atlas-gen";
          rev   = "v1.3";
          hash  = "sha256-BTRU9yETjNebIhDXnN4CxNxG/ncp7pGO96M0vKTNV7w=";
          fetchSubmodules = true;
        };
        nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
        buildInputs = [ pkgs.freetype ];
        cmakeFlags = [
          "-DMSDF_ATLAS_BUILD_STANDALONE=ON"
          "-DMSDF_ATLAS_USE_VCPKG=OFF"
          "-DMSDF_ATLAS_USE_SKIA=OFF"
          "-DMSDFGEN_DISABLE_SVG=ON"
        ];
        installPhase = ''
          mkdir -p $out/bin
          find . -name msdf-atlas-gen -type f -executable -exec cp {} $out/bin/ \;
        '';
      };
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        packages = [
          androidComposition.androidsdk
          pkgs.cmake
          pkgs.ninja
          pkgs.llvmPackages_18.clang-tools
          pkgs.android-tools   # adb
          pkgs.eigen
          pkgs.zip             # APK assembly in sh/package.sh
          pkgs.unzip
          pkgs.python3         # atlas code-gen scripts
          msdfAtlasGen         # offline MSDF atlas generation
        ];

        ANDROID_NDK_ROOT = ndkRoot;
        ANDROID_SDK_ROOT = "${androidComposition.androidsdk}/libexec/android-sdk";
        OPENXR_SDK_SRC = openxr-sdk;
        EIGEN3_INCLUDE_DIR = "${pkgs.eigen}/include/eigen3";
      };
    };
}
