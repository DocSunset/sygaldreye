# Rename all struct types to snake_case per clang-tidy config

The clang-tidy config (`readability-identifier-naming.StructCase: lower_case`) requires struct names in `lower_case`, but all structs in the codebase use `PascalCase` (`GlProgram`, `CubeMesh`, `XrSessionObj`, `Input`, `Renderer`, `EyeSwapchain`, `Scene`, `HandPose`, `CubeInstance`). This is a codebase-wide violation that appears in clang-tidy output for every file.

Renaming requires updating all include sites, forward declarations, and usages across all components. Should be done in one coordinated commit after all other structural tickets are complete, to minimize rebase conflicts.

## Acceptance
- All struct names use `lower_case` (e.g., `gl_program`, `cube_mesh`, `xr_session_obj`).
- clang-tidy `readability-identifier-naming` check passes with no struct naming warnings.
- Build succeeds; no remaining usages of old PascalCase names.
