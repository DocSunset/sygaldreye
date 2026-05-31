{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";

  outputs = { self, nixpkgs }:
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
        ];

        ANDROID_NDK_ROOT = ndkRoot;
        ANDROID_SDK_ROOT = "${androidComposition.androidsdk}/libexec/android-sdk";
      };
    };
}
