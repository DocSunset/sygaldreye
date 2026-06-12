// Copyright 2025 Travis West
#ifndef EYEBALLS_NODE_ABI_H
#define EYEBALLS_NODE_ABI_H

#define EYEBALLS_ABI_VERSION 6

/* v4: typed output emission context passed to push_outputs. */
typedef struct EyeballsOutputCtx {
    void*       store;      /* opaque: points into Graph::values */
    const char* node_id;
    void (*emit_scalar) (void* store, const char* nid, const char* port, double v);
    void (*emit_vec2)   (void* store, const char* nid, const char* port, float x, float y);
    void (*emit_vec3)   (void* store, const char* nid, const char* port, float x, float y, float z);
    void (*emit_vec4)   (void* store, const char* nid, const char* port, float x, float y, float z, float w);
    void (*emit_mat4)   (void* store, const char* nid, const char* port, const float* col16);
    void (*emit_quat)   (void* store, const char* nid, const char* port, float x, float y, float z, float w);
    void (*emit_texture)(void* store, const char* nid, const char* port,
                         unsigned int gl_id, int w, int h, unsigned int fmt, unsigned int filter);
    /* v5: draw-call values through edges; fn points at a C++ DrawFn */
    void (*emit_drawfn) (void* store, const char* nid, const char* port,
                         const void* fn);
    /* v5: CPU mesh values; mesh points at a C++ MeshPtr (shared_ptr) */
    void (*emit_mesh)   (void* store, const char* nid, const char* port,
                         const void* mesh);
    void (*emit_audio)  (void* store, const char* nid, const char* port,
                         const float* samples, int frames, int channels, int rate);
    /* v6: text values through edges (transcripts, prompts, labels) */
    void (*emit_text)   (void* store, const char* nid, const char* port,
                         const char* utf8);
    /* v6: rank-<=2 float spans (positions, transforms, lists) */
    void (*emit_span)   (void* store, const char* nid, const char* port,
                         const float* data, int rows, int cols);
} EyeballsOutputCtx;

typedef struct {
    int         version;       /* must be >= 1, current is EYEBALLS_ABI_VERSION */
    const char* type_name;     /* e.g. "water_surface" */
    const char* description;   /* human-readable, may be NULL */
    void*       (*create)   (void);
    void        (*destroy)  (void* node);
    void        (*process)  (void* node, double time_s); /* may be NULL */
    const char* (*serialize)(void* node);    /* malloc'd JSON; caller frees with free_str */
    void        (*free_str) (const char* s);
    void        (*deserialize)(void* node, const char* json);
    /* v2: push texture outputs; may be NULL; superseded by push_outputs (v4) */
    void        (*push_textures)(void* node, void* graph_ctx);
    /* v3: relative paths to source files; may be NULL */
    const char* source_header;
    const char* source_cpp;
    /* v4: rich port types; all nullable */
    const char* port_schema;                              /* static JSON, nullable */
    void        (*push_outputs)(void* node, EyeballsOutputCtx* ctx);
    void        (*push_draw_calls)(void* node, void* ctx); /* DrawCallCtx* in C++ */
    void        (*set_scalar_in) (void* node, const char* port, double v);
    void        (*set_vec2_in)   (void* node, const char* port, float x, float y);
    void        (*set_vec3_in)   (void* node, const char* port, float x, float y, float z);
    void        (*set_vec4_in)   (void* node, const char* port, float x, float y, float z, float w);
    void        (*set_mat4_in)   (void* node, const char* port, const float* col16);
    void        (*set_quat_in)   (void* node, const char* port, float x, float y, float z, float w);
    void        (*set_texture_in)(void* node, const char* port,
                                  unsigned int gl_id, int w, int h,
                                  unsigned int fmt, unsigned int filter);
    void        (*set_audio_in)  (void* node, const char* port,
                                  const float* samples, int frames, int channels, int rate);
    /* v5: fn points at a C++ DrawFn to copy; nullable */
    void        (*set_drawfn_in)(void* node, const char* port, const void* fn);
    /* v5: mesh points at a C++ MeshPtr to copy; nullable */
    void        (*set_mesh_in)  (void* node, const char* port, const void* mesh);
    /* v6: literal edges. connect points an input's src at a producer's
       owned output storage (type per port_schema kind); NULL disconnects.
       Returns 1 if the port exists and accepted the pointer. output_ptr
       returns the address of a named output's storage (stable until the
       node is destroyed), or NULL. Both nullable: legacy nodes wire
       through the set_* copy family instead. */
    int         (*connect)   (void* node, const char* port, const void* src);
    const void* (*output_ptr)(void* node, const char* port);
    /* v6: text edge delivery (writes a text input's live value) */
    void        (*set_text_in)(void* node, const char* port, const char* utf8);
} EyeballsNodeDescriptor;

/* Every plugin .so must export this symbol: */
#ifdef __cplusplus
extern "C" {
#endif
const EyeballsNodeDescriptor* eyeballs_node_descriptor(void);
#ifdef __cplusplus
}
#endif

#endif /* EYEBALLS_NODE_ABI_H */
