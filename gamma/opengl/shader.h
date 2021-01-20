#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "math/matrix.h"
#include "math/vector.h"
#include "system/traits.h"
#include "system/type_aliases.h"

namespace Gamma {
  struct GLShaderRecord {
    GLuint shader;
    std::string path;
    std::filesystem::file_time_type lastWriteTime;
  };

  GLShaderRecord Gm_CompileShader(GLenum shaderType, const char* path);
  GLShaderRecord Gm_CompileFragmentShader(const char* path);
  GLShaderRecord Gm_CompileGeometryShader(const char* path);
  GLShaderRecord Gm_CompileVertexShader(const char* path);

  class OpenGLShader : public Initable, public Destroyable {
  public:
    virtual void init() override;
    virtual void destroy() override;
    void attachShader(const GLShaderRecord& record);
    void link();
    void setBool(std::string name, bool value) const;
    void setFloat(std::string name, float value) const;
    void setInt(std::string name, int value) const;
    void setMatrix4f(std::string name, const Matrix4f& value) const;
    void setVec2f(std::string name, const Vec2f& value) const;
    void setVec3f(std::string name, const Vec3f& value) const;
    void setVec4f(std::string name, const Vec4f& value) const;
    void use();

  private:
    GLuint program = -1;
    uint32 lastFileWatchTime = 0;
    std::vector<GLShaderRecord> glShaderRecords;

    GLint getUniformLocation(const char* name) const;
    GLint getUniformLocation(std::string name) const;
  };
}