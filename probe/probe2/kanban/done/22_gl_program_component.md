# 22 — gl_program component

New component: compile/link a GLSL ES program and set uniforms. Narrow — only GLSL program lifecycle, no geometry, no XR.

## Component
- Name: `gl_program`, owning package: `render`.
- Allowed dependencies: OpenGL ES, platform (log).

## Ports
- Input `build(const char* vert, const char* frag) -> bool` — compile/link; log shader/link errors with source on failure.
- Input `use()` — `glUseProgram`.
- Input `uniform(name, Eigen::Matrix4f)` — set a mat4 uniform.
- Output `id()` / `attrib_location(name)` for callers binding vertex data.

## Scope
- Header + cpp. No on-device test (GL context required); validated via cube_mesh integration.

## Depends on
- Eigen (uniform upload type)

## Acceptance
- Used by `cube_mesh` (item 23); program links without log errors on device.
