# Prevent use of uninitialized GlProgram via factory pattern

**File:** `components/gl_program/gl_program.hpp`
**Principle:** Objects must not be constructible into an unusable state; construction and initialization must be atomic so that a successfully constructed object is always ready to use.

`GlProgram` is default-constructible with `id = 0`. Calling `use()` on an unbuilt instance calls `glUseProgram(0)`, silently disabling the shader program rather than erroring. Replace the two-phase init (`GlProgram g; g.build(...)`) with a static factory: `static std::optional<GlProgram> build(const char* vert_src, const char* frag_src)` that returns `std::nullopt` on failure. Make the default constructor `private` (or `= delete`) so callers cannot construct an unusable instance. The `CubeMesh::init()` call site must be updated to handle the optional.

## Acceptance
- `GlProgram` has no public default constructor.
- `GlProgram::build` is a `static` method returning `std::optional<GlProgram>` (or the equivalent).
- A successfully constructed `GlProgram` always has a valid `id`; `use()` is always safe to call on it.
- `CubeMesh` stores `std::optional<GlProgram>` or equivalent and handles build failure gracefully.
- Build succeeds with no new warnings.
