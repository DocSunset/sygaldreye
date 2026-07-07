# Introduce LayerSubmitter abstraction to decouple XR layer assembly from app.cpp

Deferred from Phase 9 (arch_layer_ownership_xr_session_cpp). The current callback in render_frame already partially isolates layer assembly. A proper LayerSubmitter interface would let xr_session own projection layer assembly completely, with app.cpp providing only a DrawCallback.

Implement after the geometry-agnostic renderer (062) is stable.
