#include "gl_program.hpp"
#include <android/log.h>

static const char* TAG = "eyeballs";

static GLuint compile_shader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Shader compile error: %s", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool GlProgram::build(const char* vert_src, const char* frag_src) {
    GLuint vert = compile_shader(GL_VERTEX_SHADER, vert_src);
    if (!vert) return false;
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, frag_src);
    if (!frag) { glDeleteShader(vert); return false; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Program link error: %s", log);
        glDeleteProgram(prog);
        return false;
    }
    id = prog;
    return true;
}

void GlProgram::use() const { glUseProgram(id); }

void GlProgram::uniform(const char* name, const Eigen::Matrix4f& mat) const {
    GLint loc = glGetUniformLocation(id, name);
    if (loc < 0) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "Uniform not found: %s", name);
        return;
    }
    glUniformMatrix4fv(loc, 1, GL_FALSE, mat.data());
}

GLint GlProgram::attrib_location(const char* name) const {
    return glGetAttribLocation(id, name);
}
