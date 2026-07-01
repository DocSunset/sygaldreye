# Replace int hand parameter in Input::hand_pose with a typed enum

**File:** `components/input/input.hpp`
**Principle:** Use the type system to eliminate illegal values at the call site; an `int` parameter with range [0,1] cannot be validated by the compiler, whereas an enum restricts callers to valid choices and makes the API self-documenting.

`HandPose hand_pose(int hand) const` accepts any `int`, but only values 0 (left) and 1 (right) are valid. The callers in `app.cpp` pass hardcoded literals. Introducing `enum class Hand { kLeft = 0, kRight = 1 };` and changing the signature to `HandPose hand_pose(Hand hand) const` makes invalid calls a compile error and removes the need for a runtime bounds check. The `poses_` array index becomes `static_cast<size_t>(hand)`.

## Acceptance
- `enum class Hand { kLeft = 0, kRight = 1 };` is declared (in `input.hpp` or a shared header)
- `hand_pose` signature uses `Hand` instead of `int`
- All call sites updated to use `Hand::kLeft` / `Hand::kRight`
- Build succeeds with no new warnings
