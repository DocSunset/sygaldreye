# Architecture Decision Record

## ADR-001: Target platform is Meta Quest 3

Native Android (arm64), no game engine.

## ADR-002: XR runtime is OpenXR via Meta's OpenXR Mobile SDK

Vendor-neutral API; Meta's first-class supported path. Avoids proprietary lock-in while retaining full access to Meta extensions.

## ADR-003: Graphics API is OpenGL ES 3.2 / EGL

Chosen over Vulkan for lower setup complexity and faster iteration early in development. Meta's OpenXR samples have GLES reference paths. Revisit if CPU overhead becomes a bottleneck at 90/120 Hz frame budgets.

Graphics binding: `XR_KHR_opengl_es_enable`.

## ADR-004: Activity type is NativeActivity with native_app_glue

GameActivity (Android Game Development Kit) rejected — its advantages (richer input handling) are irrelevant when input comes through OpenXR action sets. `native_app_glue` ships with the NDK; zero extra dependency.

## ADR-005: Math library is Eigen

Header-only, no build step, well-suited for the linear algebra required (vectors, matrices, quaternions for transforms and poses).
