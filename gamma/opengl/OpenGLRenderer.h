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
  struct RendererBuffers {
    OpenGLFrameBuffer gBuffer;
    OpenGLFrameBuffer indirectLight;
    OpenGLFrameBuffer reflections;
    OpenGLFrameBuffer post;
  };

  struct RendererShaders {
    OpenGLShader geometry;
    OpenGLShader emissives;
    OpenGLShader particles;
    OpenGLShader copyDepth;
    OpenGLShader pointLight;
    OpenGLShader directionalLight;
    OpenGLShader spotLight;
    OpenGLShader indirectLight;
    OpenGLShader indirectLightDenoise;
    OpenGLShader skybox;
    OpenGLShader copyFrame;
    OpenGLShader reflections;
    OpenGLShader reflectionsDenoise;
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
  };

  struct RendererContext {
    uint32 internalWidth;
    uint32 internalHeight;
    bool hasReflectiveObjects;
    bool hasRefractiveObjects;
    GLenum primitiveMode;
    std::vector<Light> pointLights;
    std::vector<Light> pointShadowCasters;
    std::vector<Light> directionalLights;
    std::vector<Light> directionalShadowcasters;
    std::vector<Light> spotLights;
    std::vector<Light> spotShadowcasters;
    Matrix4f projection;
    Matrix4f inverseProjection;
    Matrix4f view;
    Matrix4f inverseView;
    // @todo target (fbo)
  };

  class OpenGLRenderer final : public AbstractRenderer {
  public:
    OpenGLRenderer(SDL_Window* sdl_window): AbstractRenderer(sdl_window) {};
    ~OpenGLRenderer() {};

    virtual void init() override;
    virtual void destroy() override;
    virtual void render() override;
    virtual void createMesh(const Mesh* mesh) override;
    virtual void createShadowMap(const Light* light) override;
    virtual void destroyMesh(const Mesh* mesh) override;
    virtual void destroyShadowMap(const Light* light) override;
    virtual const RenderStats& getRenderStats() override;
    virtual void present() override;
    virtual void renderText(TTF_Font* font, const char* message, uint32 x, uint32 y, const Vec3f& color, const Vec4f& background) override;

  private:
    SDL_GLContext glContext;
    RendererBuffers buffers;
    RendererShaders shaders;
    RendererContext ctx;
    OpenGLLightDisc lightDisc;
    OpenGLShader screen;
    GLuint screenTexture = 0;
    uint32 frame = 0;
    std::vector<OpenGLMesh*> glMeshes;
    std::vector<OpenGLDirectionalShadowMap*> glDirectionalShadowMaps;
    std::vector<OpenGLPointShadowMap*> glPointShadowMaps;
    std::vector<OpenGLSpotShadowMap*> glSpotShadowMaps;

    struct PostShaders {
      OpenGLShader debanding;
    } post;

    void initializeRendererContext();
    void initializeLightArrays();
    void handleVsyncChanges();
    void renderSceneToGBuffer();
    void renderDirectionalShadowMaps();
    void renderPointShadowMaps();
    void renderSpotShadowMaps();
    void prepareLightingPass();
    void copyDepthInformationIntoPostBuffer();
    void renderPointLights();
    void renderPointShadowcasters();
    void renderDirectionalLights();
    void renderDirectionalShadowcasters();
    void renderSpotLights();
    void renderSpotShadowcasters();
    void renderIndirectLight();
    void renderSkybox();
    void renderParticleSystems();
    void renderReflections();
    void renderRefractiveObjects();
    void renderPostEffects();
    void renderDevBuffers();

    void renderSurfaceToScreen(SDL_Surface* surface, uint32 x, uint32 y, const Vec3f& color, const Vec4f& background);
    void writeAccumulatedEffectsBackIntoGBuffer();
  };
}