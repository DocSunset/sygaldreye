# Phase 8: GPU Texture Ports + FBO Scheduler

## Goal

Introduce a `texture` port type for GPU-to-GPU data flow between render passes. Nodes can
produce textures from FBO render passes and consume them as sampler inputs. The graph
scheduler manages FBO lifetimes and orders passes by texture dependency.

## Motivation

Some computations are better expressed as GPU passes than CPU loops. Reaction-diffusion,
fluid simulation, noise generation, and multi-pass post-processing all benefit from
staying on the GPU between frames. Once a simulation output lives in a texture, it can
feed into another node's fragment shader without a CPU round-trip.

Current state: `reaction_diffusion` runs on CPU (float arrays), uploads to mesh each frame.
Phase 8 target: runs as a GPU ping-pong pass, outputs a texture, any node can sample it.

## Port type

```cpp
struct GpuTexture {
    GLuint  id;
    int     width, height;
    GLenum  internal_format;  // e.g. GL_RGBA16F
    GLenum  filter;           // GL_LINEAR or GL_NEAREST
};
```

Declared in `outputs` using a new endpoint template:

```cpp
struct ReactionDiffusion {
    struct inputs {
        slider<"feed", "", float, 0.f, 0.1f, 0.055f> feed;
        slider<"kill", "", float, 0.f, 0.1f, 0.062f> kill;
    } inputs;
    struct outputs {
        texture_output<"concentration", GL_RGBA16F> concentration;
    } outputs;
};
```

## FBO scheduler

The frame-rate evaluation pass (Phase 5) is extended:

1. Identify all `texture_output` → `texture_input` edges in the graph
2. Topologically sort these GPU nodes by texture dependency
3. For each GPU node in order:
   - Bind its output FBO (created/resized lazily)
   - Call `node.process()`
   - Texture handle written to output port value store
4. Frame-rate CPU nodes that consume textures are scheduled after their GPU dependencies

FBO lifetime: created when a `texture_output` port is first connected, destroyed when
the edge is removed or the node is deleted. Ping-pong (two alternating FBOs for
simulation nodes that read their own previous output) is handled inside the node, not
by the scheduler.

## Serialization

`GpuTexture` port values are not serialized. They are ephemeral GPU resources.
The `inputs` sliders that parameterize the GPU computation are serialized normally.
POST `/graph` reconstructs the texture by rerunning the GPU pass from its parameters.

## Key Design Decisions

**Texture ports are not visible in the param registry.** The HTTP `/params` endpoint
only exposes scalar `inputs`. Texture connections are graph-level concerns, visible in
`/graph` but not tweakable as sliders.

**No CPU readback.** Reading a GPU texture back to CPU is expensive (~1ms+). Nodes
that need CPU access to simulation data should remain CPU-side. The texture port type
is strictly GPU-to-GPU.

**Resolution is a node concern.** The scheduler does not impose a resolution on GPU
nodes. Each node declares its output resolution (fixed, or derived from an input
texture's dimensions). The FBO is sized accordingly.

## Example: reaction-diffusion on GPU

Current path: CPU arrays → upload to mesh vertices → simple shader
Phase 8 path:
```
[rd_simulate] ──concentration──► [rd_render]
     ↑
  feed, kill (sliders)
```

`rd_simulate`: ping-pong FBO compute pass, outputs a GL_RG32F texture (U and V channels)
`rd_render`: samples the concentration texture, maps to color, outputs a `draw_call`

## Dependencies

Phase 5 (universal signal graph) — texture ports are edges in the same graph.
Phase 4 (plugin ABI) — GPU nodes satisfy the same descriptor ABI as CPU nodes.
