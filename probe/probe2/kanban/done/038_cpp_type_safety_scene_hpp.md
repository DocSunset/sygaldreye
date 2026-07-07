# Replace raw pointer + bool pairs in set_controller_poses with std::optional

**File:** `components/scene/scene.hpp`
**Principle:** Use the type system to eliminate illegal states; a `const T* ptr` paired with a `bool valid` flag can represent four states but only two are meaningful — the type system should make invalid combinations unrepresentable.

`Scene::set_controller_poses(const Eigen::Matrix4f* left_model, bool left_valid, const Eigen::Matrix4f* right_model, bool right_valid)` uses two separate pointer+bool pairs to convey optionality, which is redundant and fragile (callers can pass `left_valid=true, left_model=nullptr`). Replace both pairs with `std::optional<Eigen::Matrix4f>` parameters, letting nullopt encode "not valid" without a redundant bool. This also eliminates the separate null + bool check inside the function body.

## Acceptance
- Signature becomes `void set_controller_poses(std::optional<Eigen::Matrix4f> left_model, std::optional<Eigen::Matrix4f> right_model)`
- Implementation body simplified to `if (left_model) cubes_.push_back({*left_model});`
- Call site in `app.cpp` updated to pass `lh.valid ? std::optional{lm} : std::nullopt`
- Build succeeds with no new warnings
