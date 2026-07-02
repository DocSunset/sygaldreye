# mesh_displace GL readback → offscreen FBO pass

MeshDisplace (`mesh_nodes.cpp:42-51`) inlines glGenFramebuffers +
glReadPixels to pull a GpuTexture back to the CPU — GL outside the
boundary (ADR-008; audit §5). Re-express as a readback pass on
render_region's offscreen leg (offscreen_fbo_leg.md): the boundary
owns the FBO and surfaces the pixels as a data edge the deformer
consumes. reports/audit_conformability_editor_arc.md §5.
