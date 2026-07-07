# Extract per-frame orchestration from app.cpp into a frame_loop component

Deferred from Phase 9 (arch_single_responsibility_app_cpp). android_main currently owns XrInstance, XrSystemId, Renderer, XrSessionObj, Scene, Input, and orchestrates the frame loop. This is a god object. Extracting into a FrameLoop component would allow testing the frame logic in isolation. However, this requires creating a new component with its own CMakeLists.txt, header, source, and design doc — substantial scope. Implement as a dedicated kanban item.
