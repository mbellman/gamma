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
  struct DeferredPath {
    OpenGLLightDisc lightDisc;

    struct {
      OpenGLFrameBuffer gBuffer;
      OpenGLFrameBuffer reflections;
      OpenGLFrameBuffer post;
    } buffers;

    struct {
      OpenGLShader copyFrame;
      OpenGLShader geometry;
      OpenGLShader emissives;
      OpenGLShader copyDepth;
      OpenGLShader pointLight;
      OpenGLShader directionalLight;
      OpenGLShader reflections;
      OpenGLShader reflectionsDenoise;
      OpenGLShader skybox;
      OpenGLShader refractivePrepass;
      OpenGLShader refractiveGeometry;

      // Shadowcaster shaders
      OpenGLShader directionalShadowcaster;
      OpenGLShader directionalShadowcasterView;
      OpenGLShader pointShadowcaster;
      OpenGLShader pointShadowcasterView;
      OpenGLShader spotShadowcaster;
      OpenGLShader spotShadowcasterView;

      // Dev shaders
      OpenGLShader gBufferDev;
      OpenGLShader directionalShadowMapDev;
    } shaders;
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
    uint32 frame = 0;
    SDL_GLContext glContext;
    GLuint screenTexture = 0;
    DeferredPath deferred;
    OpenGLShader screen;
    std::vector<OpenGLMesh*> glMeshes;
    std::vector<OpenGLDirectionalShadowMap*> glDirectionalShadowMaps;
    std::vector<OpenGLPointShadowMap*> glPointShadowMaps;
    std::vector<OpenGLSpotShadowMap*> glSpotShadowMaps;

    struct PostShaders {
      OpenGLShader debanding;
    } post;

    void renderSurfaceToScreen(SDL_Surface* surface, uint32 x, uint32 y, const Vec3f& color, const Vec4f& background);
    void writeAccumulatedEffectsBackIntoGBuffer();
  };
}