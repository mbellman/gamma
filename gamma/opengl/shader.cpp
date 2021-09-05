#include <algorithm>
#include <filesystem>
#include <map>

#include "opengl/shader.h"
#include "system/console.h"
#include "system/file.h"
#include "system/flags.h"

#include "glew.h"
#include "SDL.h"

namespace Gamma {
  const static std::string INCLUDE_START = "@include('";
  const static std::string INCLUDE_END = "');";
  const static std::string INCLUDE_ROOT_PATH = "./gamma/opengl/shaders/";

  /**
   * Gm_VectorContains
   * -----------------
   */
  template<typename T>
  static bool Gm_VectorContains(const std::vector<T>& vector, T element) {
    return std::find(vector.begin(), vector.end(), element) != vector.end();
  }

  /**
   * Gm_CompileShader
   * ----------------
   *
   * @todo allow #define constants to be controlled + shader recompiled on change
   */
  GLShaderRecord Gm_CompileShader(GLenum shaderType, const char* path) {
    GLuint shader = glCreateShader(shaderType);

    std::string source = Gm_LoadFileContents(path);
    std::vector<std::string> includes;
    uint32 nextInclude;

    while ((nextInclude = source.find(INCLUDE_START)) != std::string::npos) {
      // Capture and include code from files specified by @include('...') directives
      uint32 pathStart = nextInclude + INCLUDE_START.size();
      uint32 pathEnd = source.find(INCLUDE_END, pathStart);
      // @todo store include paths in shader record;
      // check all included files when hot reloading
      std::string includePath = INCLUDE_ROOT_PATH + source.substr(pathStart, pathEnd - pathStart);
      uint32 replaceStart = nextInclude;
      uint32 replaceLength = (pathEnd + INCLUDE_END.size()) - nextInclude;

      if (Gm_VectorContains(includes, includePath)) {
        // File already included; simply remove the directive
        source.replace(replaceStart, replaceLength, "");
      } else {
        // Replace the directive with the included file contents
        std::string includeSource = Gm_LoadFileContents(includePath.c_str());

        source.replace(replaceStart, replaceLength, includeSource);
        includes.push_back(includePath);
      }
    }

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

  void OpenGLShader::fragment(const char* path) {
    attachShader(Gm_CompileFragmentShader(path));
  }

  void OpenGLShader::geometry(const char* path) {
    attachShader(Gm_CompileGeometryShader(path));
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
    #if GAMMA_DEVELOPER_MODE
      checkAndHotReloadShaders();
    #endif

    glUseProgram(program);
  }

  void OpenGLShader::vertex(const char* path) {
    attachShader(Gm_CompileVertexShader(path));
  }
}