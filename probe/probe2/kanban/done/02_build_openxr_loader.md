# 02 — build OpenXR loader for arm64

Expose the Khronos loader + headers to cmake and prove it links.

## Scope
- cmake wiring so components can `#include <openxr/openxr.h>` and link the
  loader (`DYNAMIC_LOADER=ON`, `BUILD_API_LAYERS=OFF`, `BUILD_TESTS=OFF`).
- Ensure the built `libopenxr_loader.so` path is captured for later staging
  into `lib/arm64-v8a/` (packaging is item 07).
- Throwaway TU: include the header, call `xrEnumerateApiLayerProperties`, link.

## Depends on
- 01

## Acceptance
- Throwaway TU builds + links clean for arm64 via `sh/build.sh`.
