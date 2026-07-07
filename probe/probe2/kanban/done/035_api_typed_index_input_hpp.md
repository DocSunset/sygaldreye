# Replace raw int hand index with scoped enum in Input::hand_pose

**File:** `components/input/input.hpp`
**Principle:** Array/positional indices must use scoped enums or named types, not raw integers, to prevent out-of-bounds access and document valid values at the call site.

`Input::hand_pose(int hand)` and the internal arrays `handSpaces_` and `poses_` use raw `int` indices where 0=left and 1=right. Passing any other value causes out-of-bounds access (undefined behavior). In `app.cpp`, `hand_pose(0)` and `hand_pose(1)` are called with magic numbers. Introduce `enum class Hand : int { Left = 0, Right = 1 };` in `input.hpp` and change the signature to `HandPose hand_pose(Hand hand) const`. Update all internal loops that use `int i = 0; i < 2` to iterate over `{Hand::Left, Hand::Right}` or use a helper.

## Acceptance
- `enum class Hand : int { Left = 0, Right = 1 };` is defined in `input.hpp`.
- `hand_pose` takes `Hand` instead of `int`.
- Call sites in `app.cpp` use `Hand::Left` and `Hand::Right`.
- No raw integer `0`/`1` is passed to `hand_pose` at any call site.
- Build succeeds with no new warnings.
