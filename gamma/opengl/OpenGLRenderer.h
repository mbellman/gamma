#pragma once

#include <vector>

#include "opengl/framebuffer.h"
#include "opengl/OpenGLLightDisc.h"
#include "opengl/OpenGLMesh.h"
#include "opengl/shader.h"
#include "system/AbstractRenderer.h"
#include "system/entities.h"
#include "system/type_aliases.h"

#include "SDL.h"
#include "SDL_ttf.h"

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
    virtual const RenderStats& getRenderStats() override;
    virtual void present() override;
    virtual void renderText(TTF_Font* font, const char* message, uint32 x, uint32 y) override;

  private:
    uint32 flags = 0;
    SDL_GLContext glContext;
    GLuint screenTexture = 0;
    OpenGLShader screen;
    std::vector<OpenGLMesh*> glMeshes;

    struct ForwardPath {
      // @todo (?)
      GLuint lightsUbo = 0;
    } forward;

    struct DeferredPath {
      OpenGLFrameBuffer g_buffer;
      OpenGLFrameBuffer reflections_buffer;
      OpenGLFrameBuffer post_buffer;
      OpenGLLightDisc lightDisc;
      OpenGLShader copyFrame;
      OpenGLShader geometry;
      OpenGLShader emissives;
      OpenGLShader pointLightWithoutShadow;
      OpenGLShader directionalLightWithoutShadow;
      OpenGLShader reflections;
      OpenGLShader reflectionsDenoise;
      OpenGLShader skybox;
      OpenGLShader refractiveGeometry;
      OpenGLShader debanding;
      OpenGLShader gBufferLayers;
    } deferred;

    void renderDeferred();
    void renderForward();
    void renderSurfaceToScreen(SDL_Surface* surface, uint32 x, uint32 y);
    void writeAccumulatedEffectsBackIntoGBuffer();
  };
}