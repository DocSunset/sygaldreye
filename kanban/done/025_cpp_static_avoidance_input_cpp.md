# Move mutable static 'logged' in Input::sync to an Input member

**File:** `components/input/input.cpp`
**Principle:** Mutable `static` local variables in class methods violate encapsulation and introduce hidden shared state that is not reset when a new object is constructed, preventing safe re-use and creating data races if the method is ever called concurrently.

`Input::sync()` contains `static bool logged = false;` (around line 86) which gates a one-time diagnostic log of the first valid hand position. This flag is shared across all `Input` instances and never resets, so a second `Input` object will never log. Move it to a non-static member `bool logged_ = false;` in `input.hpp`.

## Acceptance
- `static bool logged` is removed from `Input::sync()`
- `bool logged_ = false;` is added as a private member of `Input`
- Behavior is unchanged: first valid hand position is logged once per `Input` object
- Build succeeds with no new warnings
