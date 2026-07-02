# wind_animation component

Vertex-shader-based wind sway effect for grass, foliage, and other meshes.

## Components

### wind_shader

Extends the flat_mesh pattern with wind displacement uniforms.

**Vertex layout** (matches TriMesh / flat_mesh):
- Location 0: `vec3 aPos` — position
- Location 2: `vec4 aColor` — RGB = face color; **A = sway mask** (0=anchored, 1=free)

Location 1 (normal) is declared by TriMesh's VAO but not read by this shader — it is safely ignored.

**Sway mask encoding:** color.a is repurposed as a per-vertex sway weight. This avoids changing `TriVertex` or adding a second VBO. The trade-off is that `color.a` cannot simultaneously encode transparency; this is acceptable because `flat_mesh`/`wind_shader` draw opaque geometry. The fragment shader always outputs `alpha = 1.0`.

**Displacement formula:**
```
disp = wind_strength * sway_mask * sin(time * 2.1 + dot(pos.xz, wind_dir.xz) * 3.5)
offset = wind_dir * disp
```
The temporal frequency (2.1) and spatial frequency (3.5) are baked constants; they produce a natural-looking wave without additional uniforms.

### wind_animation (stamp_sway_mask)

Helper that stamps sway mask values into a `TriMeshData` based on a height threshold.

## Ports

**Inputs:**
- `set_time(float)` — elapsed time in seconds (drives animation)
- `set_wind(vec3 dir, float strength)` — wind direction (normalized) and magnitude in world units
- `set_mvp(Matrix4f)` — model-view-projection matrix

**Sources (coupled inputs):**
- `TriMesh::draw()` — must be called after `use()` / uniform setup

**Temporal couplings:**
- `create()` must be called after an EGL/GLES context is current
- `use()` must be called before setting uniforms

## Requirements

- Zero displacement when all vertex sway masks are 0
- Displacement proportional to `wind_strength`
- Wave pattern travels in `wind_dir` direction across the field
- `.cpp` files each under 100 lines of substantive code

## Allowed dependencies

- `flat_mesh` (shares build pattern; linked for potential future sharing)
- `tri_mesh` (TriMeshData, TriVertex)
- `gl_program` (GlProgram)
- Eigen, GLES3, Android log

## Owning package

`scene` (visual content helpers)
