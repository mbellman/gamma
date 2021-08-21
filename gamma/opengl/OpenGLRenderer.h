#pragma once

#include <vector>

#include "opengl/framebuffer.h"
#include "opengl/OpenGLLightDisc.h"
#include "opengl/OpenGLMesh.h"
#include "opengl/shader.h"
#include "opengl/shadowmaps.h"
#include "system/AbstractRenderer.h"
#include "system/entities.h"
#include "system/type_aliases.h"
#include "math/vector.h"

#include "SDL.h"
#include "SDL_ttf.h"

namespace Gamma {
  enum OpenGLRenderFlags {
    RENDER_DEFERRED = 1 << 0
  };

  class OpenGLRenderer final : public AbstractRenderer {
  public:
    OpenGLRenderer(SDL_Window* sdl_window): AbstractRenderer(sdl_window) {};
    ~OpenGLRenderer() {};

    virtual void init() override;
    virtual void render() override;
    virtual void destroy() override;
    virtual void createMesh(const Mesh* mesh) override;
    virtual void createShadowMap(const Light* light) override;
    virtual void destroyMesh(const Mesh* mesh) override;
    virtual void destroyShadowMap(const Light* light) override;
    virtual const RenderStats& getRenderStats() override;
    virtual void present() override;
    virtual void renderText(TTF_Font* font, const char* message, uint32 x, uint32 y, const Vec3f& color, const Vec4f& background) override;

  private:
    uint32 flags = 0;
    uint32 frame = 0;
    SDL_GLContext glContext;
    GLuint screenTexture = 0;
    OpenGLShader screen;
    std::vector<OpenGLMesh*> glMeshes;
    std::vector<OpenGLDirectionalShadowMap*> glDirectionalShadowMaps;
    std::vector<OpenGLPointShadowMap*> glPointShadowMaps;
    std::vector<OpenGLSpotShadowMap*> glSpotShadowMaps;

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
      OpenGLShader copyDepth;
      OpenGLShader pointLightWithoutShadow;
      OpenGLShader directionalLightWithoutShadow;
      OpenGLShader reflections;
      OpenGLShader reflectionsDenoise;
      OpenGLShader skybox;
      OpenGLShader refractiveGeometry;
      OpenGLShader debanding;
    } deferred;

    struct DevModeShaders {
      OpenGLShader gBufferLayers;
      OpenGLShader directionalShadowMap;
    } dev;

    struct ShadowcasterShaders {
      OpenGLShader directional;
      OpenGLShader directionalView;
      OpenGLShader point;
      OpenGLShader pointView;
      OpenGLShader spot;
      OpenGLShader spotView;
    } shadows;

    void renderDeferred();
    void renderForward();
    void renderSurfaceToScreen(SDL_Surface* surface, uint32 x, uint32 y, const Vec3f& color, const Vec4f& background);
    void writeAccumulatedEffectsBackIntoGBuffer();
  };
}