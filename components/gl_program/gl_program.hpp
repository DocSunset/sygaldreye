#pragma once
#include <GLES3/gl3.h>
#include <Eigen/Core>
#include <utility>

struct GlProgram {
    ~GlProgram() {
        if (id != 0U) { glDeleteProgram(id); }
    }

    GlProgram() = default;
    GlProgram(const GlProgram&) = delete;
    GlProgram& operator=(const GlProgram&) = delete;
    GlProgram(GlProgram&& other) noexcept : id(std::exchange(other.id, 0U)) {}
    GlProgram& operator=(GlProgram&& other) noexcept {
        if (this != &other) {
            if (id != 0U) { glDeleteProgram(id); }
            id = std::exchange(other.id, 0U);
        }
        return *this;
    }

    // Compile vert+frag, link. Returns false and logs errors on failure.
    bool build(const char* vert_src, const char* frag_src);

    void use() const;  // glUseProgram(id)

    // Set a mat4 uniform by name. No-op if not found (log warning).
    void uniform(const char* name, const Eigen::Matrix4f& mat) const;

    // Returns attribute location (-1 if not found).
    GLint attrib_location(const char* name) const;

private:
    GLuint id = 0U;
};
