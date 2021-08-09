#include <filesystem>

#include "opengl/shader.h"
#include "system/console.h"
#include "system/file.h"
#include "system/flags.h"

#include "glew.h"
#include "SDL.h"

namespace Gamma {
  /**
   * Gm_CompileShader
   * ----------------
   */
  GLShaderRecord Gm_CompileShader(GLenum shaderType, const char* path) {
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
      Console::log("[Gamma] Failed to compile shader:", path);
      Console::log(error);
    }

    auto& fsPath = std::filesystem::current_path() / path;
    auto lastWriteTime = std::filesystem::last_write_time(fsPath);

    return {
      shader,
      shaderType,
      path,
      lastWriteTime
    };
  }

  /**
   * Gm_CompileFragmentShader
   * ------------------------
   */
  GLShaderRecord Gm_CompileFragmentShader(const char* path) {
    return Gm_CompileShader(GL_FRAGMENT_SHADER, path);
  }

  /**
   * Gm_CompileGeometryShader
   * ------------------------
   */
  GLShaderRecord Gm_CompileGeometryShader(const char* path) {
    return Gm_CompileShader(GL_GEOMETRY_SHADER, path);
  }

  /**
   * Gm_CompileVertexShader
   * ----------------------
   */
  GLShaderRecord Gm_CompileVertexShader(const char* path) {
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

  void OpenGLShader::attachShader(const GLShaderRecord& record) {
    glAttachShader(program, record.shader);

    glShaderRecords.push_back(record);
  }

  void OpenGLShader::checkAndHotReloadShaders() {
    constexpr static uint32 CHECK_INTERVAL = 1000;

    if ((SDL_GetTicks() - lastShaderFileCheckTime) > CHECK_INTERVAL) {
      for (auto& record : glShaderRecords) {
        auto& fsPath = std::filesystem::current_path() / record.path;
        auto lastWriteTime = std::filesystem::last_write_time(fsPath);

        if (lastWriteTime != record.lastWriteTime) {
          glDetachShader(program, record.shader);
          glDeleteShader(record.shader);

          GLShaderRecord& updatedRecord = Gm_CompileShader(record.shaderType, record.path.c_str());

          glAttachShader(program, updatedRecord.shader);
          glLinkProgram(program);

          record = updatedRecord;

          Console::log("[Gamma] Hot-reloaded shader:", record.path);

          break;
        }
      }

      lastShaderFileCheckTime = SDL_GetTicks();
    }
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
    glUniform2fv(getUniformLocation(name), 1, &value.x);
  }

  void OpenGLShader::setVec3f(std::string name, const Vec3f& value) const {
    glUniform3fv(getUniformLocation(name), 1, &value.x);
  }

  void OpenGLShader::setVec4f(std::string name, const Vec4f& value) const {
    glUniform4fv(getUniformLocation(name), 1, &value.x);
  }

  void OpenGLShader::use() {
    #if GAMMA_HOT_RELOAD_SHADERS
      checkAndHotReloadShaders();
    #endif

    glUseProgram(program);
  }
}