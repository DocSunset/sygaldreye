# Invert the renderer–scene dependency: renderer must not depend on scene types

**File:** `components/renderer/renderer.hpp`
**Principle:** Higher-level modules (scene) must not be depended on by lower-level modules (renderer); the renderer is infrastructure, scene is content, so scene must depend on renderer — not the reverse.

`renderer.hpp` includes `scene.hpp` and declares `render_eyes(..., std::span<const CubeInstance> cubes)`. This makes the `render` package depend on the `scene` package, inverting the intended dependency direction from CLAUDE.md ("scene depends on nothing above this layer; render depends on xr and platform"). The renderer should accept an abstract draw-call interface (e.g. a callback or a `Drawable` concept) rather than a concrete scene type.

## Acceptance
- `renderer.hpp` has no include of `scene.hpp` and no reference to `CubeInstance` or any other scene type.
- `render_eyes` (or its replacement) accepts a callback or abstract interface that the scene can satisfy.
- The dependency graph has `scene` depending on `renderer` types (or neither depending on the other), not `renderer` depending on `scene`.
- The build succeeds.
