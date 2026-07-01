# terrain_generator component

Procedurally generates a colored triangle mesh for a terrain patch using fBm noise for height and a configurable color gradient (by height and slope) for the low-poly look. Covers: mountains, deserts, moon surfaces, ocean floors, arctic tundra.

## Approach

- `terrain_generator.hpp` declares:
  ```cpp
  struct TerrainParams {
      int   grid_w         = 64;   // vertices along X
      int   grid_h         = 64;   // vertices along Z
      float cell_size      = 1.0f; // metres per cell
      float height_scale   = 20.0f;
      int   octaves        = 5;
      float lacunarity     = 2.0f;
      float gain           = 0.5f;
      float noise_offset_x = 0.0f;
      float noise_offset_z = 0.0f;
      // Color bands by normalised height [0,1]
      std::array<Eigen::Vector4f, 4> band_colors; // e.g. water, grass, rock, snow
  };

  TriMeshData generate_terrain(TerrainParams const&);
  ```
- Heights are sampled from `noise::fbm`. Normals are computed per-face (cross product), then assigned to each vertex of that face so flat shading shows correct lighting.
- Color is assigned per-face by the average height of the three vertices, mapped through `band_colors`.
- `terrain_generator.test.cpp`: verify vertex/index counts match `grid_w * grid_h` and `(grid_w-1)*(grid_h-1)*2` triangles respectively; verify all normals are unit length.

## Acceptance

- A 64×64 terrain with default params produces a visually plausible mountain landscape in-headset
- Changing `noise_offset_x/Z` tiles seamlessly (no discontinuity at the edge)
- Host-buildable; tests pass
- `terrain_generator.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `noise` (item 01)
- `tri_mesh` (item 03)
