# particle_system

Fixed-capacity CPU-simulated particle pool rendered as instanced billboarded quads via GLES 3.0.

## Ports

**Inputs**
- `set_emitter(EmitterParams)` — configures spawn origin, velocity range, color/size interpolation, lifetime range, emit rate
- `update(dt, gravity)` — advances simulation by `dt` seconds; gravity applied to velocities

**Outputs**
- `draw(vp, camera_right, camera_up)` — uploads live instance data and issues one `glDrawArraysInstanced`

**Sources**
- `dt` from caller's frame timer (e.g. `scene_time`)
- `vp`, `camera_right`, `camera_up` from XR projection/view matrices

**Destinations**
- OpenGL ES 3.0 framebuffer

**Temporal couplings**
- `create_gl_resources()` called at construction; GL context must be current

**Intended seams**
- `EmitterParams` is a plain struct; callers compose emitters for fire, snow, dust, etc.

## Requirements

- Fixed pool allocated once at construction; no heap allocation in `update` or `draw`
- Ring recycler wraps dead-particle index mod capacity
- Single `glDrawArraysInstanced` call per frame
- Billboarding via `camera_right`/`camera_up` uniforms
- Color and size interpolate linearly from start to end over particle lifetime

## Allowed dependencies

- `gl_program`

## Owning package

`scene`
