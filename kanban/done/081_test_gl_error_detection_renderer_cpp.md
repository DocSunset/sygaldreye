# Add glGetError checks after GL calls in renderer

**File:** `components/renderer/renderer.cpp`
**Principle:** Wrapping every GL call with a glGetError check prevents silent error accumulation and makes failures detectable by tests.

`create_swapchains()` issues `glGenFramebuffers`, `glGenRenderbuffers`, `glBindRenderbuffer`, `glRenderbufferStorage`, `glBindFramebuffer`, `glFramebufferTexture2D`, and `glFramebufferRenderbuffer` with no `glGetError` checks (beyond `glCheckFramebufferStatus`). `render_eyes()` issues `glBindFramebuffer`, `glViewport`, `glEnable`, `glClear`, and `glBindVertexArray` with no error checks. GL errors accumulate in the error queue and manifest as mysterious downstream failures. A `GL_CHECK(call)` macro that calls `glGetError` after each GL call and logs with file/line context would make every GL failure immediately visible and testable via the on-device renderer test.

## Acceptance
- A `GL_CHECK(call)` macro (or equivalent inline helper) is defined, calling `glGetError` after `call` and logging the error code with file and line if non-zero
- All GL calls in `create_swapchains()` and `render_eyes()` are wrapped with `GL_CHECK`
- The existing renderer_test still compiles and passes
