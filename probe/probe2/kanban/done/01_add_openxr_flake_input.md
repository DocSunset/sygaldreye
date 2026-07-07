# 01 — add OpenXR-SDK flake input

Add Khronos `OpenXR-SDK` (loader + headers) as a pinned flake input.

## Scope
- Add `inputs.openxr.url = "github:KhronosGroup/OpenXR-SDK/release-1.1.x"` (pin a tag).
- Thread it into the devShell/outputs; record in `flake.lock`.

## Depends on
- nothing

## Acceptance
- `nix flake metadata` lists the openxr input; source path resolves.
