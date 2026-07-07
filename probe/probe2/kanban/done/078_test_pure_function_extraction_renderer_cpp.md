# Extract choose_swapchain_format as a pure free function

**File:** `components/renderer/renderer.cpp`
**Principle:** Pure functions (those with no I/O or mutable state) are intrinsically testable; extracting them enables offline unit tests without hardware.

The format-selection logic inside `create_swapchains()` (lines that iterate `fmts` preferring `GL_SRGB8_ALPHA8`, then `GL_RGBA8`, then first-available) is a pure policy decision fused with swapchain and FBO allocation. It cannot be tested without a live XR session and GL context. Extracting it as `int64_t choose_swapchain_format(std::span<const int64_t> formats)` yields a free function testable offline with any vector of format values, covering all three fallback branches.

## Acceptance
- A free function `choose_swapchain_format(std::span<const int64_t>)` exists in renderer.cpp (or a dedicated translation unit) and is declared in renderer.hpp or a companion header
- `create_swapchains()` calls `choose_swapchain_format` instead of inlining the selection loop
- A test (in renderer.test.cpp or a new offline test) covers: SRGB8 present → chosen, SRGB8 absent + RGBA8 present → RGBA8, neither present → first element
