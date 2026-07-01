# Implement instanced rendering for cube batch in CubeMesh

Deferred from Phase 10 (perf_batch_draw_calls_renderer_cpp). Add `draw_instanced(std::span<const Eigen::Matrix4f> mvps)` to CubeMesh: upload MVP matrices to a per-instance VBO and call glDrawElementsInstanced once per eye. Reduces draw calls from N cubes × 2 eyes to 2 draws regardless of cube count. Implement after GL optimization (074) is stable.
