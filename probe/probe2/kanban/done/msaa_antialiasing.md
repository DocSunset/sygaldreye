# MSAA anti-aliasing for eye swapchains

The rendered scene looks pixelated and geometry edges are aliased ("spiky"). Root cause: `eye_swapchain.cpp` creates swapchains with `sampleCount = 1`, ignoring the runtime's `recommendedSwapchainSampleCount` (typically 4× on Quest 3). Fix via app-side MSAA: render into a multisampled renderbuffer, then blit-resolve into the swapchain texture each frame.

## Approach

App-side MSAA keeps the swapchain at `sampleCount = 1` (swapchain images stay regular `GL_TEXTURE_2D` — no FBO attachment changes needed) and adds a resolve step:

1. **`EyeSwapchain`: add MSAA renderbuffers**
   - Read `vcv[eye].recommendedSwapchainSampleCount` and store it on `EyeSwapchain`.
   - Allocate one `GL_RGBA8` multisampled color renderbuffer and one `GL_DEPTH_COMPONENT24` multisampled depth renderbuffer per eye (not per swapchain image — they are resolved before release, so one pair suffices).
   - Allocate one extra FBO per eye (`msaa_fbo`) that attaches both multisampled renderbuffers.
   - Keep the existing per-image FBOs as resolve targets (swapchain texture + non-MSAA depth already wired).

2. **`renderer.cpp`: render to MSAA FBO, blit to resolve FBO**
   - Bind `e.msaa_fbo()` (not `e.fbo(index)`) before drawing.
   - After `on_draw(proj, v)`, blit `COLOR_BUFFER_BIT` from `msaa_fbo` to `fbo(index)` using `glBlitFramebuffer` with `GL_LINEAR` (or `GL_NEAREST` — for color MSAA resolve either works; `GL_LINEAR` is fine).
   - Depth need not be blitted (only color is submitted to the compositor).

3. **Cleanup**
   - `EyeSwapchain` destructor must delete the MSAA renderbuffers and `msaa_fbo`.
   - Move constructor/assignment must exchange the new members.

## Files affected

- `components/eye_swapchain/eye_swapchain.hpp` — add `msaa_fbo_`, `msaa_color_rb_`, `msaa_depth_rb_`, `sample_count_` members; add `msaa_fbo()` accessor.
- `components/eye_swapchain/eye_swapchain.cpp` — allocate/destroy MSAA resources in `create_swapchains`, destructor, move ctor/assign.
- `components/renderer/renderer.cpp` — bind `msaa_fbo`, draw, blit to `fbo(index)`.

## Acceptance

- Cube edges are visibly smooth compared to before.
- `recommendedSwapchainSampleCount` is used; value is logged alongside the existing resolution log.
- No GL errors in logcat.
- Swapchain `sampleCount` remains 1 (runtime-side multisampling not used).
- Each `.cpp` stays under 100 lines of substantive code.
