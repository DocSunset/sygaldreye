#pragma once
#include <GLES3/gl3.h>
#include <Eigen/Core>
#include <optional>
#include <utility>

struct GlProgram {
    ~GlProgram() {
        if (id != 0U) { glDeleteProgram(id); }
    }

    GlProgram() noexcept = default;
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

    // Compile vert+frag, link. Returns nullopt and logs errors on failure.
    static std::optional<GlProgram> build(const char* vert_src, const char* frag_src);

    void use() const;  // glUseProgram(id)

    // Set a mat4 uniform by name. No-op if not found (log warning).
    void uniform(const char* name, const Eigen::Matrix4f& mat) const;

    // Set a mat4 uniform by pre-cached location (operates on currently-bound program).
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- GLint and Matrix4f are unswappable
    static void uniform(GLint loc, const Eigen::Matrix4f& mat);

    // Returns attribute location (-1 if not found).
    [[nodiscard]] GLint attrib_location(const char* name) const;

    // Returns uniform location (-1 if not found).
    [[nodiscard]] GLint uniform_location(const char* name) const;

private:
    explicit GlProgram(GLuint prog_id) noexcept : id(prog_id) {}
    GLuint id = 0U;
};
