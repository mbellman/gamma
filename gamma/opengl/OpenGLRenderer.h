#pragma once

#include <vector>

#include "opengl/framebuffer.h"
#include "opengl/OpenGLMesh.h"
#include "opengl/shader.h"
#include "system/AbstractRenderer.h"
#include "system/entities.h"
#include "system/type_aliases.h"

namespace Gamma {
  enum OpenGLRenderFlags {
    RENDER_DEFERRED = 1 << 0,
    RENDER_SHADOWS = 1 << 1
  };

  class OpenGLRenderer final : public AbstractRenderer {
  public:
    OpenGLRenderer(SDL_Window* sdl_window): AbstractRenderer(sdl_window) {};
    ~OpenGLRenderer() {};

    virtual void init() override;
    virtual void render() override;
    virtual void destroy() override;
    virtual void createMesh(Mesh* mesh) override;
    virtual void createShadowcaster(Light* light) override;
    virtual void destroyMesh(Mesh* mesh) override;
    virtual void destroyShadowcaster(Light* light) override;

  private:
    int flags = 0;
    GLuint lightsUbo = 0;
    SDL_GLContext glContext;
    std::vector<OpenGLMesh*> glMeshes;

    struct ForwardPath {
      // @TODO
    } forward;

    struct DeferredPath {
      OpenGLFrameBuffer g_buffer;
      OpenGLShader geometry;
      OpenGLShader illumination;
      OpenGLShader emissives;
    } deferred;

    void renderDeferred();
    void renderForward();
  };
}