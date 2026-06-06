// Copyright 2025 Travis West
#ifndef EYEBALLS_NODE_ABI_H
#define EYEBALLS_NODE_ABI_H

#define EYEBALLS_ABI_VERSION 2

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
    /* v2: push texture outputs into the graph texture store; may be NULL */
    void        (*push_textures)(void* node, void* graph_ctx);
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
