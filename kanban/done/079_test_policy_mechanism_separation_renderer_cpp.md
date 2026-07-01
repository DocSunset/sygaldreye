# Separate swapchain format selection policy from GL object allocation in create_swapchains

**File:** `components/renderer/renderer.cpp`
**Principle:** Separate policy (what to choose/decide) from mechanism (how to allocate/create) so that the policy can be unit-tested without instantiating real hardware objects.

`create_swapchains()` is 80+ lines that in one monolithic function: enumerate view configs, enumerate formats, select format, create XR swapchains, enumerate swapchain images, allocate FBOs and renderbuffers, check framebuffer completeness. The format selection policy (prefer SRGB8_ALPHA8, then RGBA8, then first) cannot be tested without allocating a real XR session and GL context, because it is not separated from the mechanism. Extracting the policy into `int64_t choose_swapchain_format(std::span<const int64_t> formats)` (a pure function) and potentially a `setup_eye_fbos(EyeSwapchain&, ...)` helper for the GL side gives two independently testable units.

## Acceptance
- `choose_swapchain_format(std::span<const int64_t>)` is a standalone pure function, not a lambda or nested block
- `create_swapchains()` calls `choose_swapchain_format` and uses the result; it contains no inline format-preference logic
- A test covers the three fallback branches of `choose_swapchain_format` without requiring a GL context or XR session
