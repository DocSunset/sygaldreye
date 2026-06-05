#include "gl_program.hpp"
#include "log.hpp"
#include <array>

namespace {
constexpr const char* TAG = "gl_program";
}

static GLuint compile_shader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == 0) {
        std::array<char, 512> info_log{};
        glGetShaderInfoLog(shader, static_cast<GLsizei>(info_log.size()), nullptr, info_log.data());
        LOG_E(TAG, "Shader compile error: %s", info_log.data());
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

std::optional<GlProgram> GlProgram::build(const char* vert_src, const char* frag_src) {
    GLuint vert = compile_shader(GL_VERTEX_SHADER, vert_src);
    if (vert == 0U) { return std::nullopt; }
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, frag_src);
    if (frag == 0U) { glDeleteShader(vert); return std::nullopt; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint linked = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (linked == 0) {
        std::array<char, 512> info_log{};
        glGetProgramInfoLog(prog, static_cast<GLsizei>(info_log.size()), nullptr, info_log.data());
        LOG_E(TAG, "Program link error: %s", info_log.data());
        glDeleteProgram(prog);
        return std::nullopt;
    }
    return GlProgram(prog);
}

void GlProgram::use() const { glUseProgram(id); }

void GlProgram::uniform(const char* name, const Eigen::Matrix4f& mat) const {
    GLint loc = glGetUniformLocation(id, name);
    if (loc < 0) {
        LOG_W(TAG, "Uniform not found: %s", name);
        return;
    }
    glUniformMatrix4fv(loc, 1, GL_FALSE, mat.data());
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters,readability-convert-member-functions-to-static)
void GlProgram::uniform(GLint loc, const Eigen::Matrix4f& mat) {
    glUniformMatrix4fv(loc, 1, GL_FALSE, mat.data());
}

GLint GlProgram::attrib_location(const char* name) const {
    return glGetAttribLocation(id, name);
}

GLint GlProgram::uniform_location(const char* name) const {
    return glGetUniformLocation(id, name);
}
