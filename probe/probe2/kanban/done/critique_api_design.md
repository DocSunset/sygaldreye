# C++ API Design Critique: Eyeballs VR Application

This critique examines the C++ interfaces and component APIs of the Eyeballs Meta Quest 3 application across 8 major components. The analysis covers ownership, error handling, type design, temporal contracts, and evolution patterns.

---

## 1. STRUCT-BASED COMPOSITION WITH NO LIFECYCLE MANAGEMENT

### Problem: Bare Aggregates Without RAII

**Location:** All major components (Input, XrSessionObj, Renderer, Scene, CubeMesh, GlProgram)

Every component is defined as a C++ struct containing raw OpenGL/OpenXR handles as public members:
- `Renderer`: stores `EGLDisplay`, `EGLConfig`, `EGLContext`, `EGLSurface`, arrays of `XrSwapchain` handles
- `XrSessionObj`: stores `XrInstance`, `XrSession`, `XrSpace` handles
- `GlProgram`: stores bare `GLuint id`
- `Input`: stores `XrActionSet` and `XrAction` handles

None of these implement destructors. This means:

1. **Leak Risk**: When a component instance is destroyed or abandoned (exception, early return, or scope exit), all acquired resources remain allocated. Example in `Renderer`:
   - `eglCreateContext()` returns a handle; if initialization later fails and an exception is thrown, the context is never freed.
   - `xrCreateSwapchain()` and `glGenFramebuffers()` allocate resources that are leaked if the component goes out of scope.

2. **Use-After-Free Risk**: If code holds a component reference and the component is reassigned or copied (struct is copyable by default), the original's destructors never run, but the new instance may delete the same resources.

3. **Move Semantics Undefined**: No move constructors or move assignment operators are defined. If a component is moved into a container or returned by value, the default move-copies the handle values, leaving multiple instances pointing to the same resource.

**Why It Matters:**
- VR applications run frame loops for extended periods (minutes to hours). Resource leaks compound and degrade performance or crash the HMD.
- OpenGL and OpenXR resources are expensive; losing track of them is a critical correctness bug.

**Better Approach:**
- Define explicit destructors for all components that own resources:
  ```cpp
  struct GlProgram {
      GLuint id = 0;
      bool build(const char* vert_src, const char* frag_src);
      void use() const;
      void uniform(const char* name, const Eigen::Matrix4f& mat) const;
      GLint attrib_location(const char* name) const;
      
      ~GlProgram() { if (id) glDeleteProgram(id); }
      // Disable copy; allow move:
      GlProgram(const GlProgram&) = delete;
      GlProgram& operator=(const GlProgram&) = delete;
      GlProgram(GlProgram&&) noexcept;
      GlProgram& operator=(GlProgram&&) noexcept;
  };
  ```
- Alternatively, use `std::unique_ptr` with custom deleters for OpenGL/OpenXR handles:
  ```cpp
  struct Renderer {
      struct GLProgramDeleter {
          void operator()(GLuint id) const { if (id) glDeleteProgram(id); }
      };
      std::unique_ptr<GLuint, GLProgramDeleter> program_;
      // ... rest of members
  };
  ```

---

## 2. INCONSISTENT AND UNCLEAR OWNERSHIP SEMANTICS FOR XR HANDLES

### Problem: Implicit Lifetime Dependencies Not Enforced by API

**Location:** `xr_session.hpp`, `input.hpp`, `renderer.hpp`

XR handles (Instance, Session, Space, etc.) have strict validity windows that are not enforced by the type system:

1. **Instance Ownership**: `XrSessionObj::create()` takes `XrInstance instance` by value and stores it as a member (`instance`). The API does not express that this handle must outlive the session. If the caller creates a session, then destroys the instance (valid in principle, but wrong), the session's stored instance handle is now dangling.

2. **Implicit Synchronization Points**: `Input::sync()` requires the session, world space, and time all to be consistent with the current frame:
   ```cpp
   void Input::sync(XrSession session, XrSpace worldSpace, XrTime time);
   ```
   The signature accepts these as independent parameters, obscuring that they must all come from the same frame and refer to the same logical "render context." There's no way for a caller to know they got them wrong.

3. **Public Handle Access Coupled to Implementation**: `XrSessionObj::worldSpace_()` and `XrSessionObj::get()` expose raw `XrSpace` and `XrSession` handles. Callers reach into these directly:
   - `app.cpp` line 88: `state.xrSession.worldSpace_()`
   - `app.cpp` line 87: `state.xrSession.get()`
   
   This couples all call sites to the lifetime and validity of those handles. If the session is recreated or destroyed, all callers using stale copies will crash.

**Why It Matters:**
- OpenXR requires extremely precise synchronization: frames must use the same session, display time, and reference space. A type system that allows invalid combinations is an invitation to subtle bugs.
- Public handle exposure invites misuse: callers can pass stale or cross-frame handles without the API catching them.

**Better Approach:**
- Wrap XR handles in owned types that express lifetimes explicitly:
  ```cpp
  class XrSessionHandle {
  public:
      XrSessionHandle(XrInstance instance, XrSystemId systemId, 
                      const XrGraphicsBindingOpenGLESAndroidKHR& binding);
      ~XrSessionHandle();
      XrSession operator*() const { return handle_; }  // explicit conversion
      XrInstance instance() const { return instance_; }
      // Disallow copy; allow move
      XrSessionHandle(const XrSessionHandle&) = delete;
      XrSessionHandle& operator=(const XrSessionHandle&) = delete;
  private:
      XrInstance instance_;
      XrSession handle_;
  };
  ```
- Bind lifetimes using scoped types: frame-local context passed as a reference to ensure temporal coherence:
  ```cpp
  class XrFrameContext {
  public:
      XrFrameContext(XrSession session, XrSpace worldSpace, XrTime predictedDisplayTime);
      XrSession session() const { return session_; }
      XrSpace worldSpace() const { return worldSpace_; }
      XrTime predictedDisplayTime() const { return time_; }
  private:
      XrSession session_;
      XrSpace worldSpace_;
      XrTime time_;
  };
  
  void Input::sync(const XrFrameContext& ctx) {
      // Caller passes a frame-local context; temporal coherence guaranteed
      xrLocateSpace(handSpaces_[i], ctx.worldSpace(), ctx.predictedDisplayTime(), &loc);
  }
  ```

---

## 3. TEMPORAL COUPLING NOT EXPRESSED IN THE TYPE SYSTEM

### Problem: Initialize-Before-Use and Frame-Order Contracts Buried in Comments

**Location:** Design documents and implementation

Every design document lists temporal couplings, but they are enforced only by convention and documentation:

1. **`GlProgram`**: Must call `build()` before `use()` or `uniform()`. The header comment says so, but the API does not prevent calling `use()` on an uninitialized `GlProgram()` (id = 0). This will call `glUseProgram(0)`, which silently disables the program without error.

2. **`CubeMesh`**: Must call `init()` before `draw()`. If `draw()` is called on an uninitialized mesh (`vao_` and `vbo_` are zero), it binds VAO 0 and draws with invalid state.

3. **`XrSessionObj::render_frame()`**: The design document states: "must only be called when `session_running()` is true (after xrBeginSession, before xrEndSession)" and "render_frame() must only be called when session_running() is true." The code does not enforce this; it will happily call `xrBeginFrame()` on a non-running session, which will fail at the OpenXR level.

4. **`Input::sync()`**: The design document mandates: "sync() must be called between xrBeginFrame and xrEndFrame (OpenXR requirement for xrSyncActions)." The signature offers no way to verify the caller followed this rule. It will proceed and silently fail or produce garbage if called at the wrong time.

5. **`Renderer::create_swapchains()`**: Must be called after `init()`, after the session is created, and while the EGL context is current. If the context is not current (e.g., on a different thread), all GL calls will fail silently or hang.

**Why It Matters:**
- Frame order violations in OpenXR cause deadlock, silent rendering failure, or HMD crashes.
- Without type-level enforcement, violations surface only at runtime under specific conditions (e.g., threading bugs that happen sporadically).
- Testing cannot catch violations reliably because the test runs on the same thread.

**Better Approach:**
- Use types to linearize initialization and enforce preconditions:
  ```cpp
  class GlProgram {
  public:
      // Factory method; can fail
      static std::optional<GlProgram> build(const char* vert_src, const char* frag_src);
      
      void use() const { glUseProgram(id); }  // Only reachable if build() succeeded
      
  private:
      GLuint id;
      GlProgram(GLuint id) : id(id) {}
  };
  ```
- Use state types to enforce frame ordering:
  ```cpp
  struct FrameWaitComplete { XrFrameState state; };
  struct FrameBegun { XrFrameState state; };
  
  template <typename State>
  class XrFrameGuard {};
  
  template <>
  class XrFrameGuard<void> {  // Initial state; can wait
  public:
      XrFrameGuard<FrameWaitComplete> wait_frame() { /* ... */ }
  };
  
  template <>
  class XrFrameGuard<FrameWaitComplete> {  // Waited; can begin
  public:
      XrFrameGuard<FrameBegun> begin_frame() { /* ... */ }
  };
  
  template <>
  class XrFrameGuard<FrameBegun> {  // Frame running; can call sync/render
  public:
      void sync_input(Input& input) { input.sync(...); }
      std::vector<const XrCompositionLayerBaseHeader*> end_frame() { /* ... */ }
  };
  ```

---

## 4. INCONSISTENT ERROR HANDLING PATTERNS

### Problem: No Unified Error Model; Mix of Bool Returns, Void Throws, and Ignored Failures

**Location:** All components

1. **Bool Return Without Context**: Many functions return bool for success/failure but provide no context:
   - `Input::create()` returns false on failure but logs the error message to Android log (which may be missed by callers).
   - `Renderer::init()` returns bool; callers check it, but if init fails, which step failed? Only the log knows.
   - `Renderer::create_swapchains()` returns bool; failure is logged but provides no indication of which swapchain, which view, which format choice failed.

2. **Void Functions That Fail Silently**: 
   - `XrSessionObj::poll_events()` is void. If `xrPollEvent()` fails, it logs an error but the caller never knows the session state update was missed. The app may render stale frames even after the session should have exited.
   - `Input::sync()` is void. If `xrLocateSpace()` fails, hand poses are set to `valid=false`, which the caller must check. But if the check is forgotten, the app silently uses zero matrices for controller positions.

3. **Ignored Return Values in Call Sites**:
   - `xr.cpp` line 42: `XrResult r = xrGetSystemProperties(...)` followed by `if (XR_FAILED(r))` — but the function continues anyway. If system properties are not retrieved, the log may be missed, and the app starts with no knowledge of device capabilities.
   - `xr_session.cpp` line 83: `xrSyncActions(session, &syncInfo)` — the return value is not checked at all! If this fails (e.g., action set not attached), hand poses will be invalid, but sync() is void, so the caller never learns why.

4. **Exception Safety Undefined**: The code mixes XR error codes with no exception handling. If an exception is thrown for any reason (e.g., `std::vector::resize()` in `Renderer::create_swapchains()`), acquired EGL/OpenGL/OpenXR resources are not freed.

**Why It Matters:**
- Silent failures in a real-time frame loop accumulate. A failed input sync on one frame cascades to wrong hand poses for the entire session.
- Bool returns lose diagnostic information. "create swapchain failed: (int)result" is useless if you don't know which result code it is.
- Void error functions shift the burden to callers to check side effects (e.g., `HandPose::valid`). Callers forget, and bugs appear.

**Better Approach:**
- Use `std::expected<T, E>` or `std::optional<T>` with logging only at the caller:
  ```cpp
  struct InputCreateError {
      std::string what;  // Detailed error message
      int xr_result;
  };
  
  std::expected<Input, InputCreateError> Input::create(XrInstance instance, XrSession session) {
      // ... setup ...
      if (XR_FAILED(r)) {
          return std::unexpected(InputCreateError{
              "xrCreateActionSet failed", static_cast<int>(r)
          });
      }
      return Input{/* initialized state */};
  }
  
  // Caller:
  auto input = Input::create(instance, session);
  if (!input) {
      LOGE("Input setup failed: %s (result=%d)", input.error().what, input.error().xr_result);
      return false;
  }
  ```
- For void functions that can fail, use a result type or callback:
  ```cpp
  XrResult Input::sync(XrSession session, XrSpace worldSpace, XrTime time) {
      // ... setup ...
      XrResult r = xrLocateSpace(handSpaces_[i], worldSpace, time, &loc);
      if (XR_FAILED(r)) return r;
      // ... update poses ...
      return XR_SUCCESS;
  }
  
  // Caller:
  XrResult r = input.sync(session, worldSpace, time);
  if (XR_FAILED(r)) {
      LOGE("Input sync failed: %d", (int)r);
      // Handle gracefully or abort frame
  }
  ```

---

## 5. WIDE PUBLIC INTERFACE; INTERNAL IMPLEMENTATION DETAILS EXPOSED

### Problem: Struct Members Are Public; Implementation Changes Require Recompilation

**Location:** All components

Every component exposes implementation details as public members:

1. **`Renderer`**: Exposes public `std::array<EyeSwapchain, 2> eyes{}`. Callers can reach in and access `eyes[0].images`, `eyes[0].fbos`, etc. This couples callers to:
   - The fact that there are always exactly 2 eyes (not flexible for mono rendering or future HMD topologies).
   - The internal structure of `EyeSwapchain` (if the design changes to, say, use a std::vector instead of std::array, all callers break).
   - The lifetime of those arrays (if eyes are resized or cleared, all callers using cached pointers are broken).

2. **`EyeSwapchain`**: All members are public:
   ```cpp
   struct EyeSwapchain {
       XrSwapchain handle = XR_NULL_HANDLE;
       uint32_t    width  = 0;
       uint32_t    height = 0;
       std::vector<XrSwapchainImageOpenGLESKHR> images;
       std::vector<GLuint> fbos;
       std::vector<GLuint> depth_rbs;
       GLuint fbo(uint32_t index) const { return fbos[index]; }
   };
   ```
   The `images`, `fbos`, and `depth_rbs` should be private. Callers only need `fbo()` and possibly width/height.

3. **`XrSessionObj`**: Exposes `XrSessionState state` as a public member. Callers can read and modify it directly (though they shouldn't). The state machine is implicit: valid states are not enforced by the type system.

4. **`Input`**: Exposes `std::array<HandPose, 2> poses_{}` as a public member (though prefixed with `_`, hinting it's private—but C++ doesn't enforce this). Code in other compilation units could theoretically access `input.poses_` directly, bypassing `hand_pose()`.

**Why It Matters:**
- Refactoring the implementation requires recompiling all call sites, even if the API contract didn't change.
- Callers have no protection against misusing internal state (e.g., directly modifying `eyes[0].fbos` without synchronizing with OpenGL state).
- Future extensions (e.g., variable eye counts, dynamic resolution) cannot be implemented without breaking the API.

**Better Approach:**
- Make all implementation details private; expose only necessary queries and operations:
  ```cpp
  struct EyeSwapchain {
  public:
      uint32_t width() const { return width_; }
      uint32_t height() const { return height_; }
      GLuint fbo(uint32_t index) const { return fbos_.at(index); }
      XrSwapchain swapchain() const { return handle_; }
      
      // For internal use only (friend or private methods)
      const std::vector<XrSwapchainImageOpenGLESKHR>& images() const { return images_; }
      
  private:
      XrSwapchain handle_ = XR_NULL_HANDLE;
      uint32_t width_ = 0, height_ = 0;
      std::vector<XrSwapchainImageOpenGLESKHR> images_;
      std::vector<GLuint> fbos_;
      std::vector<GLuint> depth_rbs_;
  };
  ```
- Use accessor methods to control access and allow future changes:
  ```cpp
  class Renderer {
  public:
      uint32_t eye_width(int eye) const { return eyes_[eye].width_; }
      uint32_t eye_height(int eye) const { return eyes_[eye].height_; }
      GLuint eye_fbo(int eye, uint32_t index) const { return eyes_[eye].fbo(index); }
      XrSwapchain eye_swapchain(int eye) const { return eyes_[eye].swapchain(); }
      
  private:
      std::array<EyeSwapchain, 2> eyes_;
  };
  ```

---

## 6. CONSTRUCTOR DEFAULT INITIALIZATION ALLOWS INVALID STATES

### Problem: Default Constructors Create Half-Initialized Objects

**Location:** All components

All components use `= {}` or `= XR_NULL_HANDLE` defaults, which means a default-constructed instance is partially valid:

```cpp
struct Input {
    XrActionSet actionSet_  = XR_NULL_HANDLE;
    XrAction    poseAction_ = XR_NULL_HANDLE;
    // ... initialized handles ...
};

// This compiles:
Input input;  // actionSet_ is XR_NULL_HANDLE; safe to check, but not ready to use
input.sync(session, worldSpace, time);  // crash or silent failure
```

Problems:

1. **Invalid Use Not Caught**: `input.sync()` will call `xrSyncActions(XR_NULL_HANDLE, ...)`, which will fail at the OpenXR layer. The caller never learns that the input was never created.

2. **Move Semantics Incorrect**: A default-constructed input can be moved around without error, but it's not in a usable state. The move constructor (implicitly generated) copies null handles, which is fine, but there's no way to enforce that `sync()` isn't called before `create()`.

3. **Non-Copyable Until Explicitly Deleted**: The compiler auto-generates copy constructors and assignment operators, which just copy the handles. This is dangerous: if two Input instances point to the same action set, both will try to destroy it.

**Why It Matters:**
- Default construction in a container (e.g., `std::vector<Input> inputs(10)`) creates 10 half-initialized inputs. If any are used before being assigned, bugs occur.
- In a multi-threaded renderer, accessing an uninitialized component is a race condition that's hard to debug.

**Better Approach:**
- Use a factory pattern to ensure instances are always fully initialized:
  ```cpp
  class Input {
  public:
      // Factory: returns std::optional or std::expected
      static std::optional<Input> create(XrInstance instance, XrSession session);
      
      // No public default constructor; disallow default initialization
      Input() = delete;
      
      // Sync a fully-initialized input
      XrResult sync(XrSession session, XrSpace worldSpace, XrTime time);
      
      HandPose hand_pose(int hand) const;
      
      // Move-only:
      Input(Input&&) noexcept;
      Input& operator=(Input&&) noexcept;
      ~Input();
      
      // No copy
      Input(const Input&) = delete;
      Input& operator=(const Input&) = delete;
      
  private:
      Input(XrActionSet actionSet, XrAction poseAction, std::array<XrSpace, 2> handSpaces);
      
      XrActionSet actionSet_;
      XrAction poseAction_;
      std::array<XrSpace, 2> handSpaces_;
      std::array<HandPose, 2> poses_;
  };
  ```
  This ensures:
  - An Input can only exist if it's fully initialized.
  - No copy constructor; move-only semantics prevent accidental aliasing.
  - `sync()` is always called on a valid instance.

---

## 7. SPAN AND VECTOR CONTRACTS ARE IMPLICIT

### Problem: Lifetimes and Ownership Not Expressed by Return Types

**Location:** `Scene::cubes()`, `Renderer::render_eyes()`

1. **`Scene::cubes()` Returns `std::span<const CubeInstance>`**:
   ```cpp
   std::span<const CubeInstance> Scene::cubes() const {
       return std::span<const CubeInstance>(cubes_);
   }
   ```
   The span is valid only until the next call to `update()` or `set_controller_poses()`. The header comment says: "Outputs: `cubes()` — current set of cube instances to draw (valid until next `update`)". But the signature does not express this. A caller can:
   ```cpp
   auto cubes = scene.cubes();
   scene.update(time);  // Invalidates cubes!
   render(cubes);  // Dangling span
   ```

2. **`Renderer::render_eyes()` Fills an Output Parameter**:
   ```cpp
   bool Renderer::render_eyes(XrInstance instance, XrSession session, XrSpace refSpace, 
                              XrTime predictedDisplayTime, std::span<const CubeInstance> cubes);
   ```
   The function returns bool, but also fills `projLayer` and `projViews` (cached member variables) as a side effect. The return value indicates whether the layers are "ready" (valid to submit). But the API does not express: "After this function returns true, access the member variables `projLayer` and `projViews`; they are valid until the next call to `render_eyes()`."

**Why It Matters:**
- Callers must reverse-engineer the ownership model from code inspection and testing.
- An innocent refactor (e.g., moving `update()` call) invalidates all cached spans without warning.
- In a frame loop, using a stale span causes rendering corruption or crashes.

**Better Approach:**
- Use a return type that bundles the container with its validity lifetime:
  ```cpp
  class CubeView {
  public:
      std::span<const CubeInstance> get() const { return cubes_; }
      bool is_valid() const { return valid_; }
      
  private:
      friend class Scene;
      CubeView(const Scene& scene) : cubes_(scene.cubes_), valid_(true), scene_(&scene) {}
      
      std::span<const CubeInstance> cubes_;
      mutable bool valid_ = true;
      const Scene* scene_;
      
      // On scene mutation, invalidate:
      void invalidate() { valid_ = false; }
  };
  
  class Scene {
  public:
      CubeView cubes() const { return CubeView(*this); }
      void update(double time) {
          // Invalidate all outstanding views
          valid_views_.clear();  // or walk a list of weak_ptrs
          // ... update logic ...
      }
  };
  ```
- For output parameters filled as side effects, use an explicit "view" type:
  ```cpp
  struct RenderLayerOutput {
      bool is_valid;
      std::reference_wrapper<const XrCompositionLayerProjection> layer;
      std::reference_wrapper<const std::array<XrCompositionLayerProjectionView, 2>> views;
  };
  
  RenderLayerOutput Renderer::render_eyes(...) {
      if (!ok) return {false, {}, {}};
      return {true, std::cref(projLayer_), std::cref(projViews_)};
  }
  ```

---

## 8. LAMBDA FUNCTION SIGNATURE ALLOWS TYPE MISMATCHES

### Problem: Callback Function Type Is Loose and Unchecked

**Location:** `XrSessionObj::render_frame()`

```cpp
void render_frame(std::function<std::vector<const XrCompositionLayerBaseHeader*>(XrTime)> on_render = {});
```

The callback signature accepts `XrTime` and returns a vector of layer pointers. However:

1. **Type Erasure Hides Errors**: `std::function` uses type erasure. A lambda with a wrong signature will compile but call or cast incorrectly:
   ```cpp
   session.render_frame([&](double t) {  // Signature mismatch!
       // t is XrTime, but lambda expects double
       // Undefined behavior if XrTime is not convertible
   });
   ```

2. **Default Is Weak**: The default callback is `{}` (nullptr). If the caller doesn't provide a callback, `render_frame()` checks `if (frameState.shouldRender && on_render)`, but what if the caller accidentally constructs `std::function<...>` with a null function pointer? It will silently do nothing and return an empty layer list, which is valid but unexpected.

3. **Ownership of Returned Pointers Is Unclear**: The callback returns `std::vector<const XrCompositionLayerBaseHeader*>`. These are raw pointers to the layers. Who owns them? The comment says: "If shouldRender is false, on_render is not called." But if it is called, who is responsible for the lifetime of the layers? In the actual code (`app.cpp`), the renderer owns the layers:
   ```cpp
   return { reinterpret_cast<const XrCompositionLayerBaseHeader*>(&state.renderer.projLayer) };
   ```
   But this is fragile. If a caller returns a pointer to a stack-allocated layer, it will be dangling by the time `xrEndFrame()` is called.

**Why It Matters:**
- Mismatched callbacks lead to undefined behavior or silent wrong behavior.
- Dangling pointers in the layer array cause the HMD to render garbage or hang.

**Better Approach:**
- Use a template-based callback with enforced signature:
  ```cpp
  template <typename Fn>
  void render_frame(Fn on_render) {
      static_assert(
          std::is_invocable_v<Fn, XrTime> &&
          std::is_same_v<std::invoke_result_t<Fn, XrTime>, 
                         std::vector<const XrCompositionLayerBaseHeader*>>,
          "on_render must have signature: std::vector<const XrCompositionLayerBaseHeader*>(XrTime)"
      );
      // ...
  }
  ```
- Or use a required callback type with no default:
  ```cpp
  class RenderFrameCallback {
  public:
      // Construct from any callable with the right signature
      template <typename Fn>
      RenderFrameCallback(Fn fn) : fn_(fn) {}
      
      std::vector<const XrCompositionLayerBaseHeader*> operator()(XrTime t) const {
          return fn_(t);
      }
      
  private:
      std::function<std::vector<const XrCompositionLayerBaseHeader*>(XrTime)> fn_;
  };
  
  void render_frame(const RenderFrameCallback& on_render) {
      // on_render is always valid; no null check needed
  }
  ```

---

## 9. ARRAY INDEXING WITH UNVALIDATED PARAMETERS

### Problem: No Bounds Checking; Callers Rely on Convention

**Location:** `Input::hand_pose()`, `Scene::set_controller_poses()`

1. **`Input::hand_pose(int hand)`**:
   ```cpp
   HandPose Input::hand_pose(int hand) const { return poses_[hand]; }
   ```
   No validation that `hand` is 0 or 1. If a caller passes 2, 3, or -1, undefined behavior results. The design document says "hand: 0=left, 1=right", but the API does not enforce this.

2. **`Scene::set_controller_poses()`** is documented as taking pointers, but always accesses cubes using push_back without checking:
   ```cpp
   if (left_valid  && left_model)  cubes_.push_back({*left_model});
   if (right_valid && right_model) cubes_.push_back({*right_model});
   ```
   If callers call this multiple times without clearing cubes, the vector grows. The logic seems to always reset to 1 cube first (`cubes_.resize(1)`), but this is brittle.

**Why It Matters:**
- Out-of-bounds access in a real-time loop causes crashes or silent data corruption.
- Array-like APIs should use scoped enums or classes to prevent mistakes:
  ```cpp
  enum class Hand { Left = 0, Right = 1 };
  HandPose Input::hand_pose(Hand hand) const {
      return poses_[static_cast<int>(hand)];
  }
  ```

**Better Approach:**
- Use typed enums for indices:
  ```cpp
  enum class Hand : int { Left = 0, Right = 1 };
  
  HandPose Input::hand_pose(Hand hand) const {
      return poses_[static_cast<int>(hand)];
  }
  ```
- Or use a map or struct instead of array indexing:
  ```cpp
  struct HandPoses {
      HandPose left, right;
  };
  
  HandPoses Input::hand_poses() const {
      return {poses_[0], poses_[1]};
  }
  ```

---

## 10. IMPLICIT GLOBAL STATE AND THREAD SAFETY NOT ADDRESSED

### Problem: Static Variables and No Synchronization

**Location:** Multiple implementations

1. **`xr_session.cpp` Lines 116–150**: Static variables are used to throttle logging:
   ```cpp
   static double lastHeartbeat = 0;
   static bool firstFrame = true;
   // ...
   static double lastLocateErr = 0;
   ```
   These are not thread-safe. If `render_frame()` is called from multiple threads, races occur. The frame loop is single-threaded in `app.cpp`, but the API does not express this requirement.

2. **`input.cpp` Line 85**: A static bool `logged` is used to log only once:
   ```cpp
   static bool logged = false;
   for (int i = 0; i < 2; ++i) {
       // ...
       if (!logged) {
           LOGI("hand %d pos ...", i);
           logged = true;
       }
   }
   ```
   Again, not thread-safe, and the API does not document that `sync()` is single-threaded.

3. **`renderer.cpp` Lines 160–161**: Similar use of static bool to log once.

**Why It Matters:**
- VR rendering often involves async threads (input, compositor). If components are not thread-safe, subtle data races occur.
- The API suggests these functions are reentrant (no documentation says otherwise), inviting misuse in threaded contexts.

**Better Approach:**
- Document thread safety explicitly in headers:
  ```cpp
  /// Synchronize input state. Must be called on the render thread.
  /// \thread This function is NOT thread-safe; it must be called from a single thread.
  XrResult Input::sync(XrSession session, XrSpace worldSpace, XrTime time);
  ```
- Remove static state or protect it with locks:
  ```cpp
  XrResult Input::sync(XrSession session, XrSpace worldSpace, XrTime time) {
      static std::once_flag logged;
      // ...
      for (int i = 0; i < 2; ++i) {
          // ...
          std::call_once(logged, [i]() {
              LOGI("hand %d pos ...", i);
          });
      }
  }
  ```

---

## 11. DESIGN DOCUMENTS REVEAL INTENT NOT EXPRESSED IN CODE

### Problem: API Contract Buried in `.design.md` Files

**Location:** All `.design.md` files

Every design document specifies temporal couplings, ownership, and preconditions, but none of this is enforced in the actual API:

- `xr_session.design.md` specifies "render_frame() must only be called when session_running() is true" — not enforced by the API; the function will call xrBeginFrame anyway.
- `input.design.md` specifies "sync(session, worldSpace, time) — called each frame inside the frame loop" — the API accepts any time, any space; no validation.
- `renderer.design.md` specifies "init() must be called before xrCreateSession (EGL context needed for graphics binding)" — not enforced; the function will just fail at the xr call if called early.

The design documents are excellent (clear, detailed, specific), but they are text, not code. The compiler and runtime cannot enforce them.

**Why It Matters:**
- Developers read headers, not design docs. Contracts in design docs are often missed.
- Code review becomes harder: reviewers must cross-reference the header against the design doc to verify compliance.
- Refactoring is risky: it's easy to violate a design-doc precondition without the type system objecting.

**Better Approach:**
- Move contract information into the header as inline documentation:
  ```cpp
  /// Initiates XR session. Must be called after renderer.init() and while the EGL
  /// context is current.
  ///
  /// \param instance The XrInstance created by xr_create_instance. Must remain valid
  ///                 for the lifetime of this object.
  /// \param systemId The HMD system ID obtained from xr_get_system.
  /// \param binding The graphics binding from renderer.graphics_binding(). The EGL
  ///                context it references must be current.
  /// \return true on success; false and logs error on failure.
  /// \pre The EGL context referenced in binding must be current.
  /// \post If true is returned, this session is ready for render_frame().
  bool create(XrInstance instance, XrSystemId systemId,
              const XrGraphicsBindingOpenGLESAndroidKHR& binding);
  ```
- Use types to enforce preconditions (as discussed in sections 2 and 3).

---

## 12. SCENE API MIXES CONCERNS: ANIMATION AND CONTROLLER STATE

### Problem: Single Update Function Handles Both Time-Based Animation and Event-Based State

**Location:** `scene.hpp`, `scene.cpp`

The Scene class has two responsibilities:
1. Time-based animation: rotating a cube over time.
2. Event-based state: tracking controller poses.

But the API groups them oddly:
```cpp
void update(double time);  // Time-based; always called
void set_controller_poses(...);  // Event-based; called separately
std::span<const CubeInstance> cubes() const;  // Output; valid after update
```

In the actual call site (`app.cpp` lines 84–85):
```cpp
state.scene_.update(time_sec);
state.scene_.set_controller_poses(&lm, lh.valid, &rm, rh.valid);
```

Problems:
1. **Order Dependency**: `set_controller_poses()` must be called after `update()` within the same frame, but nothing prevents calling them in the wrong order or multiple times.
2. **Implicit Invalidation**: Each call to `update()` or `set_controller_poses()` invalidates the span from the previous call, but this is not documented.
3. **Mixed Concerns**: Animation and controller state are independent; mixing them in one class obscures this.

**Why It Matters:**
- In a complex scene, separating time-based updates from event-based state is necessary to avoid frame-order bugs.
- Future extensions (e.g., physics simulation, AI) will make this worse.

**Better Approach:**
- Separate animation and state management:
  ```cpp
  class SceneAnimator {
  public:
      void update(double time);
      std::span<const CubeInstance> primary_cubes() const;
  private:
      std::vector<CubeInstance> cubes_;
  };
  
  class SceneState {
  public:
      void set_controller_poses(const Eigen::Matrix4f* left, bool left_valid,
                                const Eigen::Matrix4f* right, bool right_valid);
      std::span<const CubeInstance> controller_cubes() const;
  private:
      std::vector<CubeInstance> cubes_;
  };
  
  // In app:
  animator.update(time_sec);
  state.set_controller_poses(...);
  
  std::vector<const CubeInstance*> all_cubes;
  for (auto& c : animator.primary_cubes()) all_cubes.push_back(&c);
  for (auto& c : state.controller_cubes()) all_cubes.push_back(&c);
  renderer.render_eyes(..., all_cubes);
  ```

---

## Summary

The Eyeballs codebase exhibits several systematic API design problems:

1. **No resource ownership enforcement** (RAII/destructors/move semantics).
2. **Temporal contracts expressed in docs, not types**.
3. **Unvalidated error returns and silent failures**.
4. **Wide public interfaces coupling callers to implementation**.
5. **Default initialization allowing invalid states**.
6. **Loose callback signatures with type erasure**.
7. **Unvalidated array indexing**.
8. **Thread safety not documented or enforced**.
9. **Design intent in markdown, not code**.
10. **Mixed concerns in single APIs**.

The patterns suggest the code was written in a "quick prototype" style, prioritizing immediate functionality over API clarity. This is understandable for a VR prototype, where getting to interactive rendering fast is valuable. However, as the codebase grows (item 28 is already extending Scene), these design flaws will compound. Frame loops are unforgiving: subtle API misuse causes crashes or hangs at runtime, especially under VR's real-time constraints.

**Key recommendation**: Invest in type-driven design using C++17+ features (`std::optional`, `std::expected`, constrained templates, scoped enums) to move verification from runtime testing to compile-time checking. The design docs are excellent; encode their contracts into the API signatures.
