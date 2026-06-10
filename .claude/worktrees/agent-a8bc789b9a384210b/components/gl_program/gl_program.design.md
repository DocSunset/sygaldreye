# gl_program

Owning package: `render`

Compiles and links OpenGL ES vertex+fragment shader pairs into a usable program object.

## Ports

- Inputs: GLSL source strings for vertex and fragment shaders
- Outputs: compiled/linked program id; attribute locations; uniform setters
- Sources: none
- Destinations: none
- Temporal couplings: `build` must be called before `use` or `uniform`
- Intended seams: none

## Requirements

- Compile vertex and fragment shaders from source strings
- Link into a program; delete shader objects on success
- Log errors (shader/program info logs) via Android log on failure
- Set mat4 uniforms by name (column-major; no transpose needed for Eigen)
- Return attribute locations by name

## Allowed dependencies

- OpenGL ES 3.2 (`GLESv3`)
- Android log (`log`)
- Eigen (math types)
