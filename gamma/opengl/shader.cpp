#include "glew.h"
#include "opengl/shader.h"
#include "system/file.h"

namespace Gamma {
  /**
   * Gm_CompileShader
   * ----------------
   */
  GLuint Gm_CompileShader(GLenum shaderType, const char* path) {
    GLuint shader = glCreateShader(shaderType);

    std::string source = Gm_LoadFileContents(path);
    const GLchar* shaderSource = source.c_str();

    glShaderSource(shader, 1, (const GLchar**)&shaderSource, 0);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status != GL_TRUE) {
      char error[512];

      glGetShaderInfoLog(shader, 512, 0, error);
      printf("[ShaderLoader] Failed to compile shader: %s\n", path);
      printf("%s\n", error);
    }

    return shader;
  }

  /**
   * Gm_CompileFragmentShader
   * ------------------------
   */
  GLuint Gm_CompileFragmentShader(const char* path) {
    return Gm_CompileShader(GL_FRAGMENT_SHADER, path);
  }

  /**
   * Gm_CompileGeometryShader
   * ------------------------
   */
  GLuint Gm_CompileGeometryShader(const char* path) {
    return Gm_CompileShader(GL_GEOMETRY_SHADER, path);
  }

  /**
   * Gm_CompileVertexShader
   * ----------------------
   */
  GLuint Gm_CompileVertexShader(const char* path) {
    return Gm_CompileShader(GL_VERTEX_SHADER, path);
  }

  /**
   * OpenGLShader
   * ------------
   */
  void OpenGLShader::init() {
    program = glCreateProgram();
  }

  void OpenGLShader::destroy() {
    glDeleteProgram(program);
  }

  void OpenGLShader::attachShader(GLuint shader) {
    glAttachShader(program, shader);
  }

  GLint OpenGLShader::getUniformLocation(const char* name) const {
    return glGetUniformLocation(program, name);
  }

  GLint OpenGLShader::getUniformLocation(std::string name) const {
    return glGetUniformLocation(program, name.c_str());
  }

  void OpenGLShader::link() {
    glLinkProgram(program);
  }

  void OpenGLShader::setBool(std::string name, bool value) const {
    setInt(name, value);
  }

  void OpenGLShader::setFloat(std::string name, float value) const {
    glUniform1f(getUniformLocation(name), value);
  }

  void OpenGLShader::setInt(std::string name, int value) const {
    glUniform1i(getUniformLocation(name), value);
  }

  void OpenGLShader::setMatrix4f(std::string name, const Matrix4f& value) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, value.m);
  }

  void OpenGLShader::setVec2f(std::string name, const Vec2f& value) const {
    glUniform2fv(getUniformLocation(name), 1, value.float2());
  }

  void OpenGLShader::setVec3f(std::string name, const Vec3f& value) const {
    glUniform3fv(getUniformLocation(name), 1, value.float3());
  }

  void OpenGLShader::use() const {
    glUseProgram(program);
  }
}