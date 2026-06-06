// Copyright 2025 Travis West
#ifndef EYEBALLS_NODE_ABI_H
#define EYEBALLS_NODE_ABI_H

#define EYEBALLS_ABI_VERSION 4

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
    void (*emit_audio)  (void* store, const char* nid, const char* port,
                         const float* samples, int frames, int channels, int rate);
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
