# Replace HandPose.valid bool with std::optional<HandPose>

**File:** `components/input/input.hpp`
**Principle:** Prefer value types and std::optional over raw pointers and boolean validity flags to make invalid states unrepresentable.

`HandPose` in `input.hpp` carries a `bool valid` field alongside `XrPosef pose`. This pattern allows callers to forget the validity check and use a stale/garbage pose silently — as happens in `app.cpp` (lines 79–82) where `lh.valid` must be manually checked before using `lh.pose`. `std::optional<HandPose>` (where `HandPose` contains only `XrPosef`) makes the absent-tracking case unrepresentable without an explicit check at the call site.

## Acceptance
- `HandPose` no longer has a `valid` field; `hand_pose()` returns `std::optional<HandPose>` (or `std::optional<XrPosef>`).
- `app.cpp` and any other call sites are updated to use the optional correctly.
- The build succeeds.
