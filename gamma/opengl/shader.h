#pragma once

#include <string>

#include "math/matrix.h"
#include "math/vector.h"
#include "system/traits.h"
#include "system/type_aliases.h"

namespace Gamma {
  GLuint Gm_CompileShader(GLenum shaderType, const char* path);
  GLuint Gm_CompileFragmentShader(const char* path);
  GLuint Gm_CompileGeometryShader(const char* path);
  GLuint Gm_CompileVertexShader(const char* path);

  class OpenGLShader : public Initable, public Destroyable {
  public:
    virtual void init() override;
    virtual void destroy() override;
    void attachShader(GLuint shader);
    void link();
    void setBool(std::string name, bool value) const;
    void setFloat(std::string name, float value) const;
    void setInt(std::string name, int value) const;
    void setMatrix4f(std::string name, const Matrix4f& value) const;
    void setVec2f(std::string name, const Vec2f& value) const;
    void setVec3f(std::string name, const Vec3f& value) const;
    void use() const;

  private:
    GLuint program = -1;

    GLint getUniformLocation(const char* name) const;
    GLint getUniformLocation(std::string name) const;
  };
}