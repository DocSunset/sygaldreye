# Document Input::sync ordering and thread-safety requirements in header

**File:** `components/input/input.hpp`
**Principle:** API contracts (preconditions, postconditions, ordering requirements) must live in the header as inline documentation, not only in separate design documents.

`input.design.md` specifies that `sync()` must be called between `xrBeginFrame` and `xrEndFrame`, that all three parameters must come from the same frame context, and that the function is not thread-safe. None of these constraints appear in `input.hpp`. Add a `///` doc comment to `sync()` stating the OpenXR requirement (must be called between begin/end frame), the temporal coherence requirement (session, worldSpace, and time must be from the same frame), and the thread-safety requirement (single-threaded only). Similarly, document that `create()` must be called before `sync()`.

## Acceptance
- `sync()` has a header doc comment covering: frame-ordering requirement, parameter coherence, thread-safety.
- `create()` has a header doc comment stating it must succeed before `sync()` is safe to call.
- Documentation is in `input.hpp`, visible without opening the design doc.
