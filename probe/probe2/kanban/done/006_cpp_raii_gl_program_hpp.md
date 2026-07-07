# Add destructor to GlProgram to delete the GL program object

**File:** `components/gl_program/gl_program.hpp`
**Principle:** Every resource acquired must have a deterministic release path — either a destructor or a smart wrapper — so that resources are freed when the owning object goes out of scope.

`GlProgram::build()` creates a program object via `glCreateProgram()` and stores its handle in `id`, but `GlProgram` has no destructor to call `glDeleteProgram(id)`. Every time a `GlProgram` is destroyed (e.g., when `CubeMesh` is destroyed), the GL program object leaks. A destructor checking `if (id) glDeleteProgram(id)` is sufficient; the copy constructor and assignment operator should also be deleted or implemented with move semantics.

## Acceptance
- `GlProgram` has a destructor that calls `glDeleteProgram(id)` when `id != 0`
- Copy constructor and copy-assignment are deleted (or move constructor/assignment implemented)
- Build succeeds with no new warnings
