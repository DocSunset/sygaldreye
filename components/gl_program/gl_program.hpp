#pragma once
#include <GLES3/gl3.h>
#include <Eigen/Core>

struct GlProgram {
    GLuint id = 0;

    // Compile vert+frag, link. Returns false and logs errors on failure.
    bool build(const char* vert_src, const char* frag_src);

    void use() const;  // glUseProgram(id)

    // Set a mat4 uniform by name. No-op if not found (log warning).
    void uniform(const char* name, const Eigen::Matrix4f& mat) const;

    // Returns attribute location (-1 if not found).
    GLint attrib_location(const char* name) const;
};
