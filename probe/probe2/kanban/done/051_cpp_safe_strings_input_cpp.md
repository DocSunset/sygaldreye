# Fix strncpy calls in Input::create to guarantee null-termination

**File:** `components/input/input.cpp`
**Principle:** C-string copy operations must always guarantee null-termination; passing the full buffer size to `strncpy` omits the terminator when the source exactly fills the buffer.

`Input::create()` calls `strncpy(asci.actionSetName, "gameplay", XR_MAX_ACTION_SET_NAME_SIZE)` and three similar calls for the action and localized names. `strncpy` copies up to N characters without adding a null terminator if the source is at least N bytes long — the OpenXR runtime then reads uninitialized memory. Each call should use `max_size - 1` as the limit and explicitly set `dest[max_size - 1] = '\0'`, or use a small helper lambda to enforce this pattern for all four sites in this file.

## Acceptance
- All four `strncpy` calls in `input.cpp` use `size - 1` as the count argument and explicitly null-terminate the destination
- No use of the raw `XR_MAX_*_SIZE` constant as a direct strncpy limit
- Build succeeds with no new warnings
