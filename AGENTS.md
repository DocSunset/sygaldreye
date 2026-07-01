Be brief. Less is more. Say less.

Write less code. The best code is no code. Every line has a cost.

Treat all warnings as errors, even if the compiler or linter does not. Fix them unless you're prepared to argue there's a good reason not to, or unless they're out of scope for your current task, in which case add a bug work item to `kanban/backlog`

# Project Standard

- The basic unit of implementation is a software component.
- A software component is embodied by its documents: design documentation, production source code, automated tests, and build automation scripts.
    - Component names use `lower_snake_case`.
    - Design documents are called `component_name.design.md`
    - Source code documents use the extensions appropriate for the source code (e.g. `.hpp`, `.cpp`)
        - Each `.cpp` file should almost always be less than 100 lines of substantive code, not including `#include`s, copyright boilerplate, and trivial code that is understood at a glance without even reading it..
        - Each `hpp/cpp` pair should define only one substantive software component (e.g. not two complex namespace-scope classes)
    - Automated tests are called `component_name.test.xxx` using the extension appropriate for the source code
    - We use cmake for build automation
    - All documents should be placed in a directory whose name is the name of the component
- Design documentation must document the following design features:
    - Ports
        - Inputs: Places where influence comes into the component that semantically belong to the component and do not depend on how other components behave
        - Outputs: Places where influence flows from the component that semantically belong to the component and do not depend on how other components behave
        - Sources: Places where influence comes into the component that depend on the behavior of another component; they are coupled inputs
        - Destinations: Places where influence flows from the component that must meet the needs of another component; they are coupled outputs
        - Temportal couplings: Places where the behavior of the component is coupled to another component's events happening in a certain order
        - Intended seams: Places where the behavior of the component is intended to be extended without modifying the component's own source code, e.g. for testing or derived components
    - Requirements (e.g. functional and non-functional and so on)
    - Allowed dependencies: an explicit list of names of components this one is allowed to depend on
    - Owning package: the name of the package to which the component belongs
- Packages are groups of components
- A package should have a `package_name.design.md` document that lists the other packages that components in the package are allowed to depend on.

# Tooling

- Prefer Nix as package manager for bringing in external dependencies
- `sh/build.sh [--clean]`: build the application
- `sh/format.sh [--check]`: auto-formatter
- `sh/lint.sh`: static analyzers
- `sh/test.sh`: build test binaries, push to a connected Quest 3 via adb, and run them on-device
- `sh/run.sh [args]`: run the application interactively

## Testing

- Use **Google Test** (sourced from `$ANDROID_NDK_ROOT/sources/third_party/googletest`) for all component test executables.
- Test files are named `component_name.test.cpp` and live in the component directory.
- Build gtest directly from its `.cc` sources; do not use its CMakeLists.
- Tests run on-device via adb, not on the host.

# Goal

TODO

# Intended Architecture

The app is a native Android VR application for Meta Quest 3. It runs as a `NativeActivity` using `native_app_glue` from the NDK. XR is managed via the OpenXR API using Meta's OpenXR Mobile SDK. Graphics use OpenGL ES 3.2 / EGL via the `XR_KHR_opengl_es_enable` extension binding. Math uses Eigen.

## Packages

### `platform`

Android entry point. Owns the `NativeActivity` lifecycle and the OpenXR instance and system. Feeds OS events and the XR instance into the rest of the app. No dependency on scene or rendering logic.

Components: `app`

### `xr`

OpenXR session management, frame loop, and input. Drives the per-frame predict/wait/begin/end cycle. Exposes rendered-layer submission and action-set input to other packages.

Depends on: `platform`

Components: `xr_session`, `input`

### `render`

OpenGL ES context (via EGL), swapchain management, and draw calls. Receives projection layer geometry from `xr` and scene content from `scene`.

Depends on: `xr`, `platform`

Components: `renderer`

### `scene`

Application content: geometry, transforms, game logic. No direct dependency on XR or rendering internals — communicates via well-defined ports.

Depends on: nothing above this layer

Components: TBD

## Data flow (per frame)

```
platform (NativeActivity events)
    └─► xr_session: xrWaitFrame / xrBeginFrame
            ├─► input: poll OpenXR action sets → scene
            └─► renderer: submit projection layers → xrEndFrame
                    └─► scene: provide draw calls
```

Architectural evolution should be documented in `adr.md`.

# Workflow

Work items are located in the `kanban` directory, one markdown file per work item. Move items to develop when you are developing them. If the item requires human review, move it to review when it is done. If it does not require human participation in the review process, move it to done when you are done working on it.

# Remember

Be brief. Less is more. Say less.

Write less code. The best code is no code. Every line has a cost.
