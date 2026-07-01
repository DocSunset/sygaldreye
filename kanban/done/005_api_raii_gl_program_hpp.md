# Add destructor and move semantics to GlProgram

**File:** `components/gl_program/gl_program.hpp`
**Principle:** Every resource acquired must have a deterministic release path; owning types must implement RAII (destructor + move, deleted copy).

`GlProgram` stores a `GLuint id` allocated by `glCreateProgram` but has no destructor, so the program object is never freed when a `GlProgram` goes out of scope or is reassigned. The default copy constructor silently duplicates the handle, so two instances will reference the same GL program. Add `~GlProgram() { if (id) glDeleteProgram(id); }`, delete the copy constructor and copy assignment, and add a move constructor and move assignment that transfer `id` and zero the source.

## Acceptance
- `GlProgram` has a destructor that calls `glDeleteProgram(id)` when `id != 0`.
- Copy constructor and copy assignment are `= delete`.
- Move constructor and move assignment are defined and zero the moved-from `id`.
- Existing call sites in `cube_mesh.cpp` compile without modification.
