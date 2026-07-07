# Move file-local statics to anonymous namespaces

clang-tidy `misc-use-anonymous-namespace` flags `static` functions and variables in `.cpp` files. Anonymous namespaces are the correct C++ idiom (they give internal linkage and avoid ODR issues). Affected locations:

- `gl_program.cpp`: `static const char* TAG`, `static GLuint compile_shader(...)`
- `cube_mesh.cpp`: `static const char* VERT`, `static const char* FRAG`
- Any other `.cpp` files using file-scope `static` for linkage control

Also fix `cppcoreguidelines-avoid-non-const-global-variables` by making `TAG` `const char* const`.

## Acceptance
- All file-local `static` functions and variables moved to `namespace { ... }` in each `.cpp`.
- `TAG` in `gl_program.cpp` is `const char* const` (or `constexpr`).
- clang-tidy `misc-use-anonymous-namespace` passes with no warnings.
