# Document Renderer initialization ordering and EGL context requirements in header

**File:** `components/renderer/renderer.hpp`
**Principle:** API contracts (preconditions, postconditions, ordering requirements) must live in the header as inline documentation, not only in separate design documents.

`renderer.design.md` specifies that `init()` must be called before `xrCreateSession` (because the EGL context is needed for the graphics binding), that `create_swapchains()` must be called after `init()` and after the session is created, and that all GL calls in `render_eyes()` require the EGL context to be current. None of these appear in `renderer.hpp`. Add `///` doc comments to `init()`, `create_swapchains()`, and `render_eyes()` that state preconditions, required call order, and thread/context requirements.

## Acceptance
- `init()` doc comment states it must be called before `xrCreateSession`.
- `create_swapchains()` doc comment states it requires `init()` to have succeeded and the session to be created.
- `render_eyes()` doc comment states the EGL context must be current.
- Documentation is in `renderer.hpp`, not only in the design doc.
