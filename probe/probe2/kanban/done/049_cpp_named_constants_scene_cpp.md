# Replace magic numbers in Scene::update with named constexpr constants

**File:** `components/scene/scene.cpp`
**Principle:** Magic numbers embedded in expressions must be replaced with named `constexpr` constants so that the intent is self-documenting and adjustments require changing only one definition.

`Scene::update()` hard-codes seven floating-point literals as animation parameters (`0.4f`, `0.19f`, `0.27f`, `0.13f`, `0.0f`, `1.1f`, `2.3f`) and three for the cube's world position (`0.0f, 1.5f, -5.0f`) and scale (`0.4f`). None carry any name explaining their role. Each should become a `constexpr float` constant at file scope with a descriptive name such as `kRotationAmplitude`, `kRotationSpeedX`, `kCubeHeight`, `kCubeDistance`, and `kCubeScale`.

## Acceptance
- All numeric literals in `Scene::update()` are replaced by named `constexpr float` constants defined at the top of `scene.cpp`
- Constant names clearly express their semantic role
- No behaviour change; build succeeds with no new warnings
