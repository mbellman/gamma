#pragma once

#include "opengl/framebuffer.h"
#include "opengl/shader.h"
#include "system/AbstractRenderer.h"
#include "system/entities.h"
#include "system/type_aliases.h"

namespace Gamma {
  enum OpenGLRenderFlags {
    DEFERRED_PATH = 1 << 0,
    SHADOWS = 1 << 1
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
    SDL_GLContext glContext;

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