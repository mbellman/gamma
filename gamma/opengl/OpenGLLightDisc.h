#pragma once

#include <vector>

#include "system/entities.h"
#include "system/traits.h"
#include "system/type_aliases.h"

namespace Gamma {
  class OpenGLLightDisc : public Initable, public Destroyable {
  public:
    virtual void init() override;
    virtual void destroy() override;
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
  };
}