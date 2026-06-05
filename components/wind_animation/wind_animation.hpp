#pragma once

struct TriMeshData;

// Sets vertex color alpha to 1.0 for vertices above height_threshold, 0.0 otherwise.
// The WindShader reads color.a as the sway mask (0=anchored, 1=free).
void stamp_sway_mask(TriMeshData& mesh, float height_threshold);
