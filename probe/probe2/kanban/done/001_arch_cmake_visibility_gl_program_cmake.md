# Fix cmake visibility: gl_program should PRIVATE-link GLESv3 and log

**File:** `components/gl_program/CMakeLists.txt`
**Principle:** CMake target dependencies must use PRIVATE unless the dependency's types appear in the component's public header; PUBLIC linkage leaks transitive dependencies to all consumers.

`gl_program/CMakeLists.txt` links `GLESv3` and `log` as PUBLIC. `gl_program.hpp` does expose `GLuint id` and `GLint` return types, which do require `GLES3/gl3.h` to be available at the include level — but `log` is purely an implementation detail in `gl_program.cpp`. The `log` library should be PRIVATE. `GLESv3` could remain PUBLIC given the exposed GL types, but this should be reviewed against whether including `GLES3/gl3.h` in the header is itself desirable (see `arch_header_isolation_gl_program_hpp`).

## Acceptance
- `log` is linked PRIVATE in `components/gl_program/CMakeLists.txt`.
- The build still succeeds.
