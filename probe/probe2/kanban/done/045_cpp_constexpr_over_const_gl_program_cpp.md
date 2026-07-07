# Use anonymous namespace and constexpr for TAG in gl_program.cpp

**File:** `components/gl_program/gl_program.cpp`
**Principle:** Prefer `constexpr` (and anonymous namespaces) over `static const` for file-scope constants to make compile-time intent explicit and to avoid potential ODR issues when translation units are linked into a shared library.

`gl_program.cpp` declares `static const char* TAG = "eyeballs";` at file scope. This is a mutable pointer to a string literal — `TAG` itself can be reassigned. The correct form is `constexpr char kTag[] = "eyeballs";` (or `constexpr std::string_view`) inside an anonymous namespace, which makes both the pointer and the data immutable and scopes the symbol properly.

## Acceptance
- `static const char* TAG` is replaced by `constexpr char kTag[] = "eyeballs";` (or `std::string_view`) inside `namespace {}`
- All uses of `TAG` updated to `kTag`
- Build succeeds with no new warnings
