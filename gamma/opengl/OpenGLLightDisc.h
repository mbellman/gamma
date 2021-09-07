#pragma once

#include <vector>

#include "system/entities.h"
#include "system/traits.h"
#include "system/type_aliases.h"

namespace Gamma {
  struct Disc {
    Vec2f offset;
    Vec2f scale;
    Light light;
  };

  class OpenGLLightDisc : public Initable, public Destroyable {
  public:
    virtual void init() override;
    virtual void destroy() override;
    void draw(const Light& light);
    void draw(const std::vector<Light>& lights);

  private:
    GLuint vao;
    /**
     * Buffers for light disc attributes.
     *
     * [0] Vertex
     * [2] Disc instances (scale, offset, light)
     */
    GLuint buffers[2];

    void configureDisc(Disc& disc, const Light& light, const Matrix4f& projection, const Matrix4f& view, float windowAspectRatio);
  };
}