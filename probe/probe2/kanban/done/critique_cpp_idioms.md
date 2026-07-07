# C++ Idiom Critique: eyeballs VR Application

## CORRECTNESS ISSUES (Severity: HIGH)

### 1. Buffer Overflow in strncpy() Calls
**File:** `components/input/input.cpp`, lines 17-18, 29-30
**File:** `components/app/xr.cpp`, lines 93-94

**Problem:** Multiple uses of `strncpy()` with XR constant string size limits. While `strncpy()` prevents overflow, the code doesn't ensure null-termination. More critically, the size parameter should be `XR_MAX_ACTION_SET_NAME_SIZE - 1` and similar to reserve space for the null terminator:

```cpp
strncpy(asci.actionSetName, "gameplay", XR_MAX_ACTION_SET_NAME_SIZE);  // UNSAFE
```

The OpenXR API expects null-terminated strings. If `XR_MAX_ACTION_SET_NAME_SIZE` is used as the copy limit without subtracting 1, the result is NOT null-terminated.

**Remedy:** Use `std::string` and copy only `size - 1` characters, OR use a safer wrapper:
```cpp
auto copy_string = [](char* dest, size_t max_size, const char* src) {
    strncpy(dest, src, max_size - 1);
    dest[max_size - 1] = '\0';
};
copy_string(asci.actionSetName, XR_MAX_ACTION_SET_NAME_SIZE, "gameplay");
```

---

### 2. Out-of-Bounds Array Access in Input::hand_pose()
**File:** `components/input/input.cpp`, line 104

**Problem:** Function accepts `int hand` parameter with no bounds checking:
```cpp
HandPose Input::hand_pose(int hand) const { return poses_[hand]; }
```

If called with `hand` outside [0, 1], this is undefined behavior. The caller in `app.cpp` lines 79-80 calls with hardcoded 0 and 1, but this is fragile and violates defensive programming.

**Remedy:** Either:
1. Use `std::array::at()` for bounds checking: `return poses_.at(hand);`
2. Add explicit bounds check: `assert(hand >= 0 && hand < 2);`
3. Change signature to take an enum: `enum Hand { LEFT = 0, RIGHT = 1 };` then use `poses_[static_cast<size_t>(hand)]`

---

### 3. Uninitialized Variables and Potential Null Dereferences
**File:** `components/app/xr.cpp`, line 66

**Problem:** Pointer returned from `xrGetInstanceProcAddr()` is not null-checked before use:
```cpp
PFN_xrGetOpenGLESGraphicsRequirementsKHR getReqs = nullptr;
xrGetInstanceProcAddr(instance, "xrGetOpenGLESGraphicsRequirementsKHR",
                      (PFN_xrVoidFunction*)&getReqs);
if (getReqs) { ... }
```

While the code does check later, the function pointer is cast through `(PFN_xrVoidFunction*)&getReqs` before assignment. This is a C-style reinterpret_cast violation and potentially unsafe.

**File:** `components/scene/scene.cpp`, lines 25-29

**Problem:** Null pointer dereference risk:
```cpp
void Scene::set_controller_poses(const Eigen::Matrix4f* left_model, bool left_valid,
                                 const Eigen::Matrix4f* right_model, bool right_valid) {
    ...
    if (left_valid  && left_model)  cubes_.push_back({*left_model});  // Safe: checks both
    if (right_valid && right_model) cubes_.push_back({*right_model});
}
```

While this particular code IS correct (it checks both `left_valid` and `left_model`), the parameter design is fragile. The `bool left_valid` should be implicit in the pointer nullness; passing both is redundant.

**Remedy:** Use `std::optional<std::reference_wrapper<const Eigen::Matrix4f>>` or similar:
```cpp
void Scene::set_controller_poses(
    std::optional<std::reference_wrapper<const Eigen::Matrix4f>> left_model,
    std::optional<std::reference_wrapper<const Eigen::Matrix4f>> right_model);
```

---

### 4. Static Variable Data Races in Frame Loop
**File:** `components/xr_session/xr_session.cpp`, lines 116-117, 142, 150
**File:** `components/renderer/renderer.cpp`, lines 160, 174

**Problem:** `static` variables modified across multiple frame iterations without synchronization:
```cpp
void XrSessionObj::render_frame(...) {
    static double lastHeartbeat = 0;
    static bool firstFrame = true;
    ...
    if (firstFrame) { LOG("frame loop running"); firstFrame = false; }
    ...
    if (t - lastHeartbeat >= 5.0) { LOG("frame loop heartbeat"); lastHeartbeat = t; }
}
```

If `render_frame()` is called from multiple threads (or re-entered), these statics introduce data races. While Android typically runs a single render thread, this violates thread-safety principles and will fail under concurrent access or re-entrancy.

**Remedy:** Move statics to class members:
```cpp
struct XrSessionObj {
    double lastHeartbeat_ = 0;
    bool firstFrame_ = true;
    ...
};
```

**File:** `components/renderer/renderer.cpp`, line 160, 219

Similar issues with `static bool first`, `static bool layerLogged`.

---

### 5. Memory Leak in CubeMesh
**File:** `components/cube_mesh/cube_mesh.cpp`, lines 71-94

**Problem:** The Element Buffer Object (EBO) is created but never deleted:
```cpp
void CubeMesh::init() {
    GLuint ebo = 0;
    glGenBuffers(1, &ebo);          // Created
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(IDX), IDX, GL_STATIC_DRAW);
    glBindVertexArray(0);
    // ebo is never stored, never deleted -> LEAK
}
```

The VAO (`vao_`) holds a reference to the EBO while bound, so the EBO won't be freed even if VAO is deleted. The EBO handle is lost, and the GPU memory is never reclaimed.

**Remedy:** Store EBO as a member or delete it after VAO is finalized:
```cpp
struct CubeMesh {
    unsigned vao_ = 0, vbo_ = 0, ebo_ = 0;
    ...
};

// In destructor:
~CubeMesh() {
    if (ebo_) glDeleteBuffers(1, &ebo_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
}
```

---

## RESOURCE MANAGEMENT ISSUES (Severity: MEDIUM-HIGH)

### 6. No RAII Wrappers for OpenGL Resources
**File:** `components/renderer/renderer.hpp`, lines 28-37

**Problem:** EGL and GL handles are stored as bare types without RAII:
```cpp
struct Renderer {
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLConfig  config  = nullptr;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;
    ...
};
```

No destructor cleans up these resources. If the app exits or an exception occurs (if exceptions were enabled), these resources leak. The `Renderer` object has no explicit lifetime management.

**Remedy:** Implement RAII:
```cpp
struct Renderer {
    // ... members ...
    
    ~Renderer() {
        if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
        if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
        if (display != EGL_NO_DISPLAY) eglTerminate(display);
    }
    
    // Delete copy constructor/assignment to prevent double-free
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    
    // Move semantics if needed
    Renderer(Renderer&& other) noexcept : display(std::exchange(other.display, EGL_NO_DISPLAY)), ... {}
};
```

---

### 7. No RAII Wrappers for OpenXR Handles
**File:** `components/xr_session/xr_session.hpp`, lines 14-19

**Problem:** XrSession, XrSpace, XrInstance handles lack cleanup:
```cpp
struct XrSessionObj {
    XrInstance   instance  = XR_NULL_HANDLE;
    XrSession    handle    = XR_NULL_HANDLE;
    XrSpace      worldSpace = XR_NULL_HANDLE;
    ...
};
```

No destructor calls `xrDestroySpace()`, `xrDestroySession()`, etc. These resources are leaked when the struct is destroyed.

**Remedy:** Implement proper cleanup:
```cpp
struct XrSessionObj {
    ...
    ~XrSessionObj() {
        if (worldSpace != XR_NULL_HANDLE && handle != XR_NULL_HANDLE) {
            xrDestroySpace(handle, worldSpace);
        }
        if (handle != XR_NULL_HANDLE) {
            xrDestroySession(handle);
        }
        // Note: instance lifetime is managed elsewhere
    }
    
    XrSessionObj(const XrSessionObj&) = delete;
    XrSessionObj& operator=(const XrSessionObj&) = delete;
};
```

---

### 8. No RAII Wrappers for Input Resources
**File:** `components/input/input.hpp`, lines 22-24

**Problem:** XrActionSet and XrAction handles are never destroyed:
```cpp
struct Input {
    ...
private:
    XrActionSet actionSet_  = XR_NULL_HANDLE;
    XrAction    poseAction_ = XR_NULL_HANDLE;
    ...
};
```

These should be destroyed in a destructor via `xrDestroyAction()` and `xrDestroyActionSet()`.

**Remedy:** Add cleanup:
```cpp
~Input() {
    if (poseAction_ != XR_NULL_HANDLE && actionSet_ != XR_NULL_HANDLE) {
        xrDestroyAction(actionSet_, poseAction_);
    }
    if (actionSet_ != XR_NULL_HANDLE) {
        xrDestroyActionSet(actionSet_);
    }
}
```

---

### 9. No RAII for Swapchain Resources
**File:** `components/renderer/renderer.hpp`, lines 19-20

**Problem:** Swapchain images and framebuffers are never destroyed:
```cpp
struct EyeSwapchain {
    std::vector<XrSwapchainImageOpenGLESKHR> images;
    std::vector<GLuint> fbos;
    std::vector<GLuint> depth_rbs;
    ...
};
```

No destructor calls `glDeleteFramebuffers()` or `glDeleteRenderbuffers()` or `xrDestroySwapchain()`.

**Remedy:** Add destructor to `EyeSwapchain`:
```cpp
struct EyeSwapchain {
    ...
    ~EyeSwapchain() {
        if (!fbos.empty()) glDeleteFramebuffers(fbos.size(), fbos.data());
        if (!depth_rbs.empty()) glDeleteRenderbuffers(depth_rbs.size(), depth_rbs.data());
        if (handle != XR_NULL_HANDLE) {
            xrDestroySwapchain(handle);
        }
    }
};
```

---

## CONST CORRECTNESS ISSUES (Severity: MEDIUM)

### 10. Missing const in Scene::cubes()
**File:** `components/scene/scene.hpp`, line 12

Already correct: `std::span<const CubeInstance> cubes() const;` ✓

### 11. Missing const in Input
**File:** `components/input/input.hpp`, lines 18, 19

These are correct. However, consider:

**File:** `components/input/input.cpp`, lines 85-86

The loop variable `i` should use `size_t`:
```cpp
for (int i = 0; i < 2; ++i) {  // WRONG: int for array size
```

---

## TYPE SAFETY & MODERN C++ ISSUES (Severity: MEDIUM)

### 12. Use of C-Style Arrays Instead of std::array
**File:** `components/xr_session/xr_session.cpp`, lines 24, 88, 172

```cpp
XrPath handPaths[2];  // C array
```

Should be:
```cpp
std::array<XrPath, 2> handPaths;
```

**File:** `components/cube_mesh/cube_mesh.cpp`, lines 28, 62

```cpp
static const float VERTS[24 * 6] = { ... };
static const unsigned short IDX[36] = { ... };
```

These are static const, which is acceptable, but `std::array` would be safer:
```cpp
static constexpr std::array<float, 144> VERTS = { ... };
static constexpr std::array<unsigned short, 36> IDX = { ... };
```

---

### 13. Unsafe Pointer Arithmetic and Casts
**File:** `components/cube_mesh/cube_mesh.cpp`, lines 89-91

```cpp
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 24, (void*)0);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);
```

Magic numbers `24` and `12` are hard-coded byte offsets. These should be named constants or computed:
```cpp
constexpr size_t VERTEX_STRIDE = 6 * sizeof(float);  // 6 floats per vertex
constexpr size_t COLOR_OFFSET = 3 * sizeof(float);   // RGB at offset 12
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_STRIDE, (void*)0);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_STRIDE, (void*)(COLOR_OFFSET));
```

---

### 14. C-Style Casts in Unsafe Contexts
**File:** `components/app/xr.cpp`, line 68

```cpp
(PFN_xrVoidFunction*)&getReqs  // C-style cast through void*
```

Should use `reinterpret_cast`:
```cpp
reinterpret_cast<PFN_xrVoidFunction*>(&getReqs)
```

**File:** `components/xr_session/xr_session.cpp`, line 32

```cpp
auto& sc = *reinterpret_cast<XrEventDataSessionStateChanged*>(&ev);  // UNSAFE
```

This is a reinterpret_cast of a union-like buffer to a derived type. If `ev.type` is not `XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED`, this is undefined behavior. The code relies on the OpenXR API's contract that the buffer matches the type field. Consider:
```cpp
if (ev.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
    auto& sc = *reinterpret_cast<XrEventDataSessionStateChanged*>(&ev);
    // safe
}
```

Already done here, but worth noting it's on the knife's edge.

---

### 15. int Used for Unsigned Quantities
**File:** `components/input/input.cpp`, lines 65, 86

```cpp
for (int i = 0; i < 2; ++i) {  // OK for 2, but principle violated
```

**File:** `components/renderer/renderer.cpp`, lines 106, 189, 222

```cpp
for (int eye = 0; eye < 2; ++eye) {  // OK but should be size_t
```

**File:** `components/xr_session/xr_session.cpp`, lines 93

```cpp
for (uint32_t i = 0; i < count; i++) {  // Correct, but inconsistent with other loops
```

**Remedy:** Use `size_t` or `std::ptrdiff_t` consistently for loop indices:
```cpp
for (size_t i = 0; i < 2; ++i) {
```

---

### 16. Magic Numbers Without Named Constants
**File:** `components/scene/scene.cpp`, lines 5-17

```cpp
float t = static_cast<float>(time);
float ax = 0.4f * sinf(t * 0.19f + 0.0f);
float ay = 0.4f * sinf(t * 0.27f + 1.1f);
float az = 0.4f * sinf(t * 0.13f + 2.3f);
```

These are animation parameters. Should be named:
```cpp
constexpr float ROTATION_AMPLITUDE = 0.4f;
constexpr float ROTATION_SPEED_X = 0.19f;
constexpr float ROTATION_SPEED_Y = 0.27f;
constexpr float ROTATION_SPEED_Z = 0.13f;
constexpr float PHASE_X = 0.0f;
constexpr float PHASE_Y = 1.1f;
constexpr float PHASE_Z = 2.3f;
```

**File:** `components/scene/scene.cpp`, line 14

```cpp
Eigen::Translation3f(0.0f, 1.5f, -5.0f)
```

Magic position. Should be named:
```cpp
constexpr float CUBE_HEIGHT = 1.5f;
constexpr float CUBE_DISTANCE = 5.0f;
```

**File:** `components/renderer/renderer.cpp`, line 207

```cpp
Eigen::Matrix4f proj = projection(views[eye].fov, 0.05f, 100.0f);
```

Near/far plane: should be named constants.

**File:** `components/app/app.cpp`, lines 67-69, 72

```cpp
constexpr float HAND_CUBE_OFFSET_X_CM =  0.0f;  // Good, already named
constexpr float HAND_CUBE_OFFSET_Y_CM =  0.0f;
constexpr float HAND_CUBE_OFFSET_Z_CM =  0.0f;
scale_m(0,0) = scale_m(1,1) = scale_m(2,2) = 0.035f;  // Not named
```

The `0.035f` scale is a magic number. Should be:
```cpp
constexpr float HAND_CUBE_SCALE = 0.035f;
scale_m(0,0) = scale_m(1,1) = scale_m(2,2) = HAND_CUBE_SCALE;
```

---

### 17. Inconsistent Use of static vs. constexpr
**File:** `components/cube_mesh/cube_mesh.cpp`, lines 28, 62

```cpp
static const float VERTS[24 * 6] = { ... };
static const unsigned short IDX[36] = { ... };
```

Should be `constexpr` for clarity that these are compile-time constants:
```cpp
static constexpr float VERTS[] = { ... };  // std::size(VERTS) == 144
static constexpr unsigned short IDX[] = { ... };
```

---

## UNDEFINED & UNSPECIFIED BEHAVIOR (Severity: MEDIUM)

### 18. Uninitialized Stack Arrays
**File:** `components/xr_session/xr_session.cpp`, line 88

```cpp
XrReferenceSpaceType types[16] = {};  // Good: zero-initialized
```

This is correct. However:

**File:** `components/renderer/renderer.cpp`, line 81

```cpp
XrViewConfigurationView vcv[2]{};  // Good: zero-initialized
```

Correct.

**File:** `components/renderer/renderer.cpp`, lines 172

```cpp
XrView views[2]{{XR_TYPE_VIEW},{XR_TYPE_VIEW}};  // Good: explicit init
```

Correct.

However, compare with:

**File:** `components/gl_program/gl_program.cpp`, line 13

```cpp
char log[512];  // UNINITIALIZED!
glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
```

This is actually safe because `glGetShaderInfoLog()` null-terminates the output, but it violates the principle that log buffers should be zeroed for safety:
```cpp
char log[512] = {};  // Better: zeroed
```

---

### 19. Potential Division by Zero
**File:** `components/renderer/renderer.cpp`, lines 17-20

```cpp
m(0,0) = 2.f * near_z / (r - l);  // Division without checking r != l
m(1,1) = 2.f * near_z / (t - b);  // Division without checking t != b
m(2,2) = -(far_z + near_z) / (far_z - near_z);  // Division without checking
m(2,3) = -2.f * far_z * near_z / (far_z - near_z);
```

If the FOV is malformed or `near_z == far_z`, these divisions produce NaN or inf. No guards.

**Remedy:** Add assertions or early returns:
```cpp
Eigen::Matrix4f projection(const XrFovf& fov, float near_z, float far_z) {
    assert(near_z > 0 && far_z > near_z);
    assert(fov.angleLeft != fov.angleRight);  // Not strictly accurate but principle
    
    float l = std::tan(fov.angleLeft)  * near_z;
    float r = std::tan(fov.angleRight) * near_z;
    float b = std::tan(fov.angleDown)  * near_z;
    float t = std::tan(fov.angleUp)    * near_z;
    
    if (r == l || t == b || far_z == near_z) {
        LOGE("Invalid projection parameters");
        return Eigen::Matrix4f::Identity();
    }
    ...
}
```

---

### 20. std::vector Implicit Behavior with Resize and Push
**File:** `components/scene/scene.cpp`, lines 17-18, 27-29

```cpp
void Scene::update(double time) {
    ...
    cubes_.resize(1);
    cubes_[0].model = xf.matrix();
}

void Scene::set_controller_poses(...) {
    cubes_.resize(1);
    if (left_valid  && left_model)  cubes_.push_back({*left_model});
    if (right_valid && right_model) cubes_.push_back({*right_model});
}
```

This pattern is problematic: `resize(1)` followed by `push_back()` results in a vector of size 2 or 3, not what might be expected. The code is correct but semantically odd.

More importantly, if `update()` is called after `set_controller_poses()`, the controller poses are lost because `resize(1)` truncates the vector.

This suggests the design is fragile. Consider separating scene and controller rendering:
```cpp
struct Scene {
    std::vector<CubeInstance> world_cubes;
    std::optional<CubeInstance> left_controller;
    std::optional<CubeInstance> right_controller;
    
    void update(double time);
    std::vector<const CubeInstance*> all_cubes() const;
};
```

---

## EXCEPTION SAFETY (Severity: MEDIUM)

### 21. No Exception Safety Analysis
**File:** Entire codebase

The code uses exceptions-disabled settings (implied by lack of try-catch blocks), but there's no explicit documentation of this assumption. The code violates the C++ Core Guideline of explicit exception safety contracts.

While disabling exceptions is acceptable in embedded/VR contexts, the rationale should be documented:
- Add a comment at the top of each major component
- Document in a build configuration file or README

**Remedy:** Add a file `EXCEPTION_POLICY.md` or similar:
```
This project compiles with -fno-exceptions and -fno-rtti.
All allocations and resource acquisitions must be failure-proof or use
manual error codes. No code relies on exception unwinding.
```

---

## OTHER ISSUES (Severity: LOW-MEDIUM)

### 22. Potential Integer Overflow in Time Calculations
**File:** `components/app/app.cpp`, line 63

```cpp
double time_sec = static_cast<double>(t) * 1e-9;  // t is XrTime (int64_t)
```

This is correct, but on line 63 there's an implicit assumption that the conversion won't overflow. For `int64_t` max (≈9.2e18), the result is ≈9.2e9 seconds ≈ 292 years, which is safe for this app's lifetime.

No issue here, but worth noting.

---

### 23. Implicit Conversion from uint32_t to int
**File:** `components/renderer/renderer.cpp`, line 138

```cpp
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
    GL_TEXTURE_2D, e.images[i].image, 0);
```

`e.images[i].image` is likely `uint32_t` from the OpenXR spec, but `glFramebufferTexture2D` expects `GLuint` (which is typically `unsigned int`). This is probably fine, but the types are not perfectly aligned.

**Remedy:** Explicit cast:
```cpp
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
    GL_TEXTURE_2D, static_cast<GLuint>(e.images[i].image), 0);
```

---

### 24. Using `endl` Would Be Better Than `\n` for Flushing (Minor)
**File:** `components/hello/hello.cpp`, line 5

```cpp
out << "hello, world\n";
```

For a greeting, this is fine. But in general, for logs that should flush immediately:
```cpp
out << "hello, world" << std::endl;  // Flushes
```

Not critical here, but noted.

---

### 25. Potential Thread Safety in Static Variables Across DLL Boundaries
**File:** `components/gl_program/gl_program.cpp`, line 4

```cpp
static const char* TAG = "eyeballs";
```

If this component is compiled as a shared library and used from multiple translation units, the static might be duplicated. Not a practical issue on Android, but worth noting.

**Remedy:** Use an anonymous namespace:
```cpp
namespace {
    constexpr std::string_view TAG = "eyeballs";
}
```

---

## RECOMMENDED COMPILER FLAGS & STATIC ANALYSIS

Based on `.clang-tidy` configuration (already good), add these to `CMakeLists.txt`:

```cmake
target_compile_options(eyeballs PRIVATE
    -Wall -Wextra -Wpedantic
    -Wshadow -Wconversion -Wsign-conversion
    -Wnon-virtual-dtor -Woverloaded-virtual
    -fno-exceptions -fno-rtti
    -Wl,--as-needed  # Link only used symbols
)

# Enable AddressSanitizer in debug builds
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(eyeballs PRIVATE -fsanitize=address)
    target_link_options(eyeballs PRIVATE -fsanitize=address)
endif()
```

---

## SUMMARY TABLE

| Issue | File(s) | Severity | Type |
|-------|---------|----------|------|
| strncpy buffer not null-terminated | input.cpp, xr.cpp | HIGH | Correctness |
| Out-of-bounds hand_pose() access | input.cpp:104 | HIGH | Correctness |
| Static data races in render loop | xr_session.cpp, renderer.cpp | HIGH | Correctness |
| Memory leak: EBO not deleted | cube_mesh.cpp | HIGH | Resource |
| No RAII for EGL resources | renderer.hpp | HIGH | Resource |
| No RAII for OpenXR handles | xr_session.hpp, input.hpp | HIGH | Resource |
| No RAII for swapchain resources | renderer.hpp | HIGH | Resource |
| C arrays instead of std::array | xr_session.cpp, cube_mesh.cpp | MEDIUM | Type Safety |
| Magic numbers in shader offsets | cube_mesh.cpp | MEDIUM | Clarity |
| Uninitialized shader log buffer | gl_program.cpp:13 | MEDIUM | Safety |
| Division by zero in projection | vr_math.cpp | MEDIUM | Correctness |
| Fragile vector resize pattern | scene.cpp | MEDIUM | Design |
| No exception safety documentation | All | MEDIUM | Documentation |
| Magic numbers throughout | Multiple | LOW-MEDIUM | Clarity |

---

## PRIORITY ACTION ITEMS

1. **IMMEDIATE:** Implement RAII wrappers for EGL, OpenXR, and GL resources to prevent leaks
2. **IMMEDIATE:** Fix strncpy() calls to ensure null-termination
3. **SOON:** Eliminate static variables in frame loops; move to class members
4. **SOON:** Add bounds checking to `Input::hand_pose()`
5. **SOON:** Fix CubeMesh EBO leak with proper cleanup
6. **SOON:** Replace magic numbers with named constants
7. **ONGOING:** Convert C arrays to `std::array<>`
8. **ONGOING:** Add explicit exception safety / no-exception policy documentation

