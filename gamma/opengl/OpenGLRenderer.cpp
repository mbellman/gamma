#include <algorithm>
#include <cstdio>
#include <map>

#include "opengl/errors.h"
#include "opengl/indirect_buffer.h"
#include "opengl/OpenGLRenderer.h"
#include "opengl/OpenGLScreenQuad.h"
#include "opengl/renderer_setup.h"
#include "system/AbstractController.h"
#include "system/AbstractScene.h"
#include "system/camera.h"
#include "system/console.h"
#include "system/entities.h"
#include "system/flags.h"
#include "system/Window.h"

#include "SDL.h"
#include "SDL_ttf.h"
#include "glew.h"
#include "SDL_opengl.h"

namespace Gamma {
  const static uint32 MAX_LIGHTS = 1000;
  const static Vec4f FULL_SCREEN_TRANSFORM = { 0.0f, 0.0f, 1.0f, 1.0f };

  /**
   * OpenGLRenderer
   * --------------
   */
  void OpenGLRenderer::init() {
    internalResolution = { 1920, 1080 };

    // Initialize OpenGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    glContext = SDL_GL_CreateContext(sdl_window);
    glewExperimental = true;

    glewInit();

    SDL_GL_SetSwapInterval(0);

    // Initialize global buffers
    Gm_InitDrawIndirectBuffer();

    #ifndef GAMMA_DEVELOPER_MODE
      // Set uniform shader constants upfront, since
      // shaders won't change during runtime
      Vec2f screenSize((float)internalResolution.width, (float)internalResolution.height);

      shaders.indirectLight.setVec2f("screenSize", screenSize);
      shaders.indirectLightComposite.setVec2f("screenSize", screenSize);
      shaders.reflectionsDenoise.setVec2f("screenSize", screenSize);
      shaders.refractivePrepass.setVec2f("screenSize", screenSize);
      shaders.refractiveGeometry.setVec2f("screenSize", screenSize);

      // @todo set sampler2D texture units
    #endif

    // Initialize screen texture
    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Initialize renderer
    Gm_InitRendererResources(buffers, shaders, internalResolution);

    lightDisc.init();

    // Initialize post shaders
    post.debanding.init();
    post.debanding.vertex("./gamma/opengl/shaders/quad.vert.glsl");
    post.debanding.fragment("./gamma/opengl/shaders/deband.frag.glsl");
    post.debanding.link();

    // Initialize remaining shaders
    screen.init();
    screen.vertex("./gamma/opengl/shaders/quad.vert.glsl");
    screen.fragment("./gamma/opengl/shaders/screen.frag.glsl");
    screen.link();

    // Enable default OpenGL settings
    glEnable(GL_PROGRAM_POINT_SIZE);
  }

  void OpenGLRenderer::destroy() {
    Gm_DestroyRendererResources(buffers, shaders);
    Gm_DestroyDrawIndirectBuffer();

    lightDisc.destroy();

    glDeleteTextures(1, &screenTexture);

    SDL_GL_DeleteContext(glContext);
  }

  void OpenGLRenderer::render() {
    if (AbstractScene::active == nullptr) {
      return;
    }

    initializeRendererContext();
    initializeLightArrays();
    handleSettingsChanges();
    renderSceneToGBuffer();

    if (Gm_IsFlagEnabled(GammaFlags::RENDER_SHADOWS)) {
      if (glDirectionalShadowMaps.size() > 0) {
        renderDirectionalShadowMaps();
      }

      if (glPointShadowMaps.size() > 0) {
        renderPointShadowMaps();
      }

      if (glSpotShadowMaps.size() > 0) {
        renderSpotShadowMaps();
      }
    }

    prepareLightingPass();

    if (
      ctx.directionalLights.size() == 0 &&
      ctx.directionalShadowcasters.size() == 0 &&
      (
        ctx.hasReflectiveObjects ||
        ctx.hasRefractiveObjects ||
        Gm_IsFlagEnabled(GammaFlags::RENDER_GLOBAL_ILLUMINATION) ||
        Gm_IsFlagEnabled(GammaFlags::RENDER_AMBIENT_OCCLUSION)
      )
    ) {
      copyDepthIntoAccumulationBuffer();
    }

    if (ctx.pointLights.size() > 0) {
      renderPointLights();
    }

    if (ctx.pointShadowCasters.size() > 0) {
      renderPointShadowcasters();
    }

    if (ctx.directionalLights.size() > 0) {
      renderDirectionalLights();
    }

    if (ctx.directionalShadowcasters.size() > 0) {
      renderDirectionalShadowcasters();
    }

    if (ctx.spotLights.size() > 0) {
      renderSpotLights();
    }

    if (ctx.spotShadowcasters.size() > 0) {
      renderSpotShadowcasters();
    }

    if (ctx.hasEmissiveObjects) {
      copyEmissiveObjects();
    }

    if (
      Gm_IsFlagEnabled(GammaFlags::RENDER_AMBIENT_OCCLUSION) ||
      Gm_IsFlagEnabled(GammaFlags::RENDER_GLOBAL_ILLUMINATION) ||
      Gm_IsFlagEnabled(GammaFlags::RENDER_INDIRECT_SKY_LIGHT)
    ) {
      renderIndirectLight();
    }

    glDisable(GL_BLEND);

    renderSkybox();

    // @todo if (ctx.hasParticleSystems)
    renderParticleSystems();

    if (ctx.hasReflectiveObjects && Gm_IsFlagEnabled(GammaFlags::RENDER_REFLECTIONS)) {
      renderReflections();
    }

    if (ctx.hasRefractiveObjects && Gm_IsFlagEnabled(GammaFlags::RENDER_REFRACTIVE_GEOMETRY)) {
      renderRefractiveGeometry();
    }

    renderPostEffects();

    #if GAMMA_DEVELOPER_MODE
      if (Gm_IsFlagEnabled(GammaFlags::RENDER_DEV_BUFFERS)) {
        renderDevBuffers();
      }
    #endif

    frame++;
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::initializeRendererContext() {
    auto& camera = *Camera::active;

    // Accumulation buffers
    ctx.accumulationSource = &buffers.accumulation1;
    ctx.accumulationTarget = &buffers.accumulation2;

    // Render dimensions/primitive type
    ctx.internalWidth = internalResolution.width;
    ctx.internalHeight = internalResolution.height;
    ctx.primitiveMode = Gm_IsFlagEnabled(GammaFlags::WIREFRAME_MODE) ? GL_LINES : GL_TRIANGLES;

    // Camera projection/view/inverse matrices
    ctx.projection = Matrix4f::glPerspective(internalResolution, 45.0f, 1.0f, 10000.0f).transpose();
    ctx.previousViews[1] = ctx.previousViews[0];
    ctx.previousViews[0] = ctx.view;

    ctx.view = (
      Matrix4f::rotation(camera.orientation.toVec3f()) *
      Matrix4f::translation(camera.position.invert().gl())
    ).transpose();

    ctx.inverseProjection = ctx.projection.inverse();
    ctx.inverseView = ctx.view.inverse();

    // Track special object types
    ctx.hasEmissiveObjects = false;
    ctx.hasReflectiveObjects = false;
    ctx.hasRefractiveObjects = false;

    for (auto* glMesh : glMeshes) {
      if (glMesh->getObjectCount() > 0) {
        if (glMesh->isMeshType(MeshType::EMISSIVE)) {
          ctx.hasEmissiveObjects = true;
        } else if (glMesh->isMeshType(MeshType::REFLECTIVE)) {
          ctx.hasReflectiveObjects = true;
        } else if (glMesh->isMeshType(MeshType::REFRACTIVE)) {
          ctx.hasRefractiveObjects = true;
        }
      }
    }
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::initializeLightArrays() {
    ctx.pointLights.clear();
    ctx.pointShadowCasters.clear();
    ctx.directionalLights.clear();
    ctx.directionalShadowcasters.clear();
    ctx.spotLights.clear();
    ctx.spotShadowcasters.clear();

    for (auto& light : AbstractScene::active->getLights()) {
      switch (light.type) {
        case LightType::POINT:
          ctx.pointLights.push_back(light);
          break;
        case LightType::POINT_SHADOWCASTER:
          if (Gm_IsFlagEnabled(GammaFlags::RENDER_SHADOWS)) {
            ctx.pointShadowCasters.push_back(light);
          } else {
            ctx.pointLights.push_back(light);
          }

          break;
        case LightType::DIRECTIONAL:
          ctx.directionalLights.push_back(light);
          break;
        case LightType::DIRECTIONAL_SHADOWCASTER:
          if (Gm_IsFlagEnabled(GammaFlags::RENDER_SHADOWS)) {
            ctx.directionalShadowcasters.push_back(light);
          } else {
            ctx.directionalLights.push_back(light);
          }

          break;
        case LightType::SPOT:
          ctx.spotLights.push_back(light);
          break;
        case LightType::SPOT_SHADOWCASTER:
          if (Gm_IsFlagEnabled(GammaFlags::RENDER_SHADOWS)) {
            ctx.spotShadowcasters.push_back(light);
          } else {
            ctx.spotLights.push_back(light);
          }

          break;
      }
    }
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::handleSettingsChanges() {
    if (Gm_FlagWasEnabled(GammaFlags::VSYNC)) {
      SDL_GL_SetSwapInterval(1);

      #if GAMMA_DEVELOPER_MODE
        Console::log("[Gamma] V-Sync enabled");
      #endif
    } else if (Gm_FlagWasDisabled(GammaFlags::VSYNC)) {
      SDL_GL_SetSwapInterval(0);

      #if GAMMA_DEVELOPER_MODE
        Console::log("[Gamma] V-Sync disabled");
      #endif
    }

    if (Gm_FlagWasEnabled(GammaFlags::RENDER_AMBIENT_OCCLUSION)) {
      shaders.indirectLight.define("USE_SCREEN_SPACE_AMBIENT_OCCLUSION", "1");
      shaders.indirectLightComposite.define("USE_AVERAGE_INDIRECT_LIGHT", "1");
    } else if (Gm_FlagWasDisabled(GammaFlags::RENDER_AMBIENT_OCCLUSION)) {
      shaders.indirectLight.define("USE_SCREEN_SPACE_AMBIENT_OCCLUSION", "0");

      if (!Gm_IsFlagEnabled(GammaFlags::RENDER_GLOBAL_ILLUMINATION)) {
        shaders.indirectLightComposite.define("USE_AVERAGE_INDIRECT_LIGHT", "0");
      }
    }

    if (Gm_FlagWasEnabled(GammaFlags::RENDER_GLOBAL_ILLUMINATION)) {
      shaders.indirectLight.define("USE_SCREEN_SPACE_GLOBAL_ILLUMINATION", "1");
      shaders.indirectLightComposite.define("USE_AVERAGE_INDIRECT_LIGHT", "1");
    } else if (Gm_FlagWasDisabled(GammaFlags::RENDER_GLOBAL_ILLUMINATION)) {
      shaders.indirectLight.define("USE_SCREEN_SPACE_GLOBAL_ILLUMINATION", "0");

      if (!Gm_IsFlagEnabled(GammaFlags::RENDER_AMBIENT_OCCLUSION)) {
        shaders.indirectLightComposite.define("USE_AVERAGE_INDIRECT_LIGHT", "0");
      }
    }

    if (Gm_FlagWasEnabled(GammaFlags::RENDER_INDIRECT_SKY_LIGHT)) {
      shaders.indirectLightComposite.define("USE_INDIRECT_SKY_LIGHT", "1");
    } else if (Gm_FlagWasDisabled(GammaFlags::RENDER_INDIRECT_SKY_LIGHT)) {
      shaders.indirectLightComposite.define("USE_INDIRECT_SKY_LIGHT", "0");
    }
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::renderSceneToGBuffer() {
    buffers.gBuffer.write();

    glViewport(0, 0, ctx.internalWidth, ctx.internalHeight);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glCullFace(GL_BACK);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 0xFF, 0xFF);
    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);

    auto& camera = *Camera::active;

    shaders.geometry.use();
    shaders.geometry.setMatrix4f("projection", ctx.projection);
    shaders.geometry.setMatrix4f("view", ctx.view);
    shaders.geometry.setInt("meshTexture", 0);
    shaders.geometry.setInt("meshNormalMap", 1);

    // Render emissive objects
    glStencilMask(MeshType::EMISSIVE);

    for (auto* glMesh : glMeshes) {
      if (glMesh->isMeshType(MeshType::EMISSIVE)) {
        shaders.geometry.setBool("hasTexture", glMesh->hasTexture());
        shaders.geometry.setBool("hasNormalMap", glMesh->hasNormalMap());

        glMesh->render(ctx.primitiveMode);
      }
    }

    // Render reflective objects
    glStencilMask(MeshType::REFLECTIVE);

    for (auto* glMesh : glMeshes) {
      if (glMesh->isMeshType(MeshType::REFLECTIVE)) {
        shaders.geometry.setBool("hasTexture", glMesh->hasTexture());
        shaders.geometry.setBool("hasNormalMap", glMesh->hasNormalMap());

        glMesh->render(ctx.primitiveMode);
      }
    }

    // Render non-emissive, non-reflective objects
    glStencilMask(MeshType::NON_EMISSIVE);

    for (auto* glMesh : glMeshes) {
      if (glMesh->isMeshType(MeshType::NON_EMISSIVE)) {
        shaders.geometry.setBool("hasTexture", glMesh->hasTexture());
        shaders.geometry.setBool("hasNormalMap", glMesh->hasNormalMap());

        glMesh->render(ctx.primitiveMode);
      }
    }

    glDisable(GL_STENCIL_TEST);
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::renderDirectionalShadowMaps() {
    auto& camera = *Camera::active;
    auto& shader = shaders.directionalShadowcasterView;

    shader.use();

    for (uint32 mapIndex = 0; mapIndex < glDirectionalShadowMaps.size(); mapIndex++) {
      auto& glShadowMap = *glDirectionalShadowMaps[mapIndex];
      auto& light = ctx.directionalShadowcasters[mapIndex];

      glShadowMap.buffer.write();

      for (uint32 cascade = 0; cascade < 3; cascade++) {
        glShadowMap.buffer.writeToAttachment(cascade);
        Matrix4f lightView = Gm_CreateCascadedLightViewMatrixGL(cascade, light.direction, camera);

        shader.setMatrix4f("lightView", lightView);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // @todo glMultiDrawElementsIndirect for static world geometry
        // (will require a handful of other changes to mesh organization/data buffering)
        for (auto* glMesh : glMeshes) {
          auto* sourceMesh = glMesh->getSourceMesh();

          if (sourceMesh->canCastShadows && sourceMesh->maxCascade >= cascade) {
            glMesh->render(ctx.primitiveMode, true);
          }
        }
      }
    }
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::renderPointShadowMaps() {
    const static Vec3f directions[6] = {
      Vec3f(-1.0f, 0.0f, 0.0f),
      Vec3f(1.0f, 0.0f, 0.0f),
      Vec3f(0.0f, -1.0f, 0.0f),
      Vec3f(0.0f, 1.0f, 0.0f),
      Vec3f(0.0f, 0.0f, -1.0f),
      Vec3f(0.0f, 0.0f, 1.0f)
    };

    const static Vec3f tops[6] = {
      Vec3f(0.0f, -1.0f, 0.0f),
      Vec3f(0.0f, -1.0f, 0.0f),
      Vec3f(0.0f, 0.0f, 1.0f),
      Vec3f(0.0f, 0.0f, -1.0f),
      Vec3f(0.0f, -1.0f, 0.0f),
      Vec3f(0.0f, -1.0f, 0.0f)
    };

    auto& shader = shaders.pointShadowcasterView;

    shader.use();

    for (uint32 mapIndex = 0; mapIndex < glPointShadowMaps.size(); mapIndex++) {
      auto& glShadowMap = *glPointShadowMaps[mapIndex];
      auto& light = ctx.pointShadowCasters[mapIndex];

      if (light.isStatic && glShadowMap.isRendered) {
        continue;
      }

      glShadowMap.buffer.write();

      glClear(GL_DEPTH_BUFFER_BIT);

      for (uint32 i = 0; i < 6; i++) {
        Matrix4f projection = Matrix4f::glPerspective({ 1024, 1024 }, 90.0f, 1.0f, light.radius);
        Matrix4f view = Matrix4f::lookAt(light.position.gl(), directions[i], tops[i]);
        Matrix4f lightMatrix = (projection * view).transpose();

        shader.setMatrix4f("lightMatrices[" + std::to_string(i) + "]", lightMatrix);
      }

      shader.setVec3f("lightPosition", light.position.gl());
      shader.setFloat("farPlane", light.radius);

      // @todo glMultiDrawElementsIndirect for static world geometry
      // (will require a handful of other changes to mesh organization/data buffering)
      for (auto* glMesh : glMeshes) {
        auto* sourceMesh = glMesh->getSourceMesh();

        if (sourceMesh->canCastShadows) {
          glMesh->render(ctx.primitiveMode, true);
        }
      }

      glShadowMap.isRendered = true;
    }
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::renderSpotShadowMaps() {
    auto& shader = shaders.spotShadowcasterView;

    shader.use();

    for (uint32 mapIndex = 0; mapIndex < glSpotShadowMaps.size(); mapIndex++) {
      auto& glShadowMap = *glSpotShadowMaps[mapIndex];
      auto& light = ctx.spotShadowcasters[mapIndex];

      if (light.isStatic && glShadowMap.isRendered) {
        continue;
      }

      Matrix4f lightProjection = Matrix4f::glPerspective({ 1024, 1024 }, 120.0f, 1.0f, light.radius);
      Matrix4f lightView = Matrix4f::lookAt(light.position.gl(), light.direction.invert().gl(), Vec3f(0.0f, 1.0f, 0.0f));
      Matrix4f lightMatrix = (lightProjection * lightView).transpose();

      glShadowMap.buffer.write();

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      shader.setMatrix4f("lightMatrix", lightMatrix);

      // @todo glMultiDrawElementsIndirect for static world geometry
      // (will require a handful of other changes to mesh organization/data buffering)
      for (auto* glMesh : glMeshes) {
        auto* sourceMesh = glMesh->getSourceMesh();

        if (sourceMesh->canCastShadows) {
          glMesh->render(ctx.primitiveMode, true);
        }
      }

      glShadowMap.isRendered = true;
    }
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::prepareLightingPass() {
    buffers.gBuffer.read();
    ctx.accumulationTarget->write();

    glViewport(0, 0, ctx.internalWidth, ctx.internalHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_LESS, MeshType::PARTICLE_SYSTEM, 0xFF);
    glStencilMask(0x00);
  }

  /**
   * When no directional lights/shadowcasters are available,
   * we perform a screen pass to copy depth information for
   * all on-screen geometry into the target accumulation buffer.
   * If we don't do this, reflections and refractions will miss
   * any surfaces which aren't directly illuminated, i.e. otherwise
   * written to the accumulation buffer in the lighting passes.
   */
  void OpenGLRenderer::copyDepthIntoAccumulationBuffer() {
    auto& shader = shaders.copyDepth;

    shader.use();
    shader.setVec4f("transform", FULL_SCREEN_TRANSFORM);
    shader.setInt("color_and_depth", 0);

    OpenGLScreenQuad::render();
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::renderPointLights() {
    auto& camera = *Camera::active;
    auto& shader = shaders.pointLight;

    shader.use();
    shader.setInt("colorAndDepth", 0);
    shader.setInt("normalAndSpecularity", 1);
    shader.setVec3f("cameraPosition", camera.position);
    shader.setMatrix4f("inverseProjection", ctx.inverseProjection);
    shader.setMatrix4f("inverseView", ctx.inverseView);

    lightDisc.draw(ctx.pointLights);
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::renderPointShadowcasters() {
    auto& camera = *Camera::active;
    auto& shader = shaders.pointShadowcaster;

    shader.use();
    shader.setInt("colorAndDepth", 0);
    shader.setInt("normalAndSpecularity", 1);
    shader.setInt("shadowMap", 3);
    shader.setVec3f("cameraPosition", camera.position);
    shader.setMatrix4f("inverseProjection", ctx.inverseProjection);
    shader.setMatrix4f("inverseView", ctx.inverseView);

    for (uint32 i = 0; i < ctx.pointShadowCasters.size(); i++) {
      auto& glShadowMap = *glPointShadowMaps[i];
      auto& light = ctx.pointShadowCasters[i];

      glShadowMap.buffer.read();
      lightDisc.draw(light);
    }
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::renderDirectionalLights() {
    auto& camera = *Camera::active;
    auto& shader = shaders.directionalLight;

    shader.use();
    shader.setVec4f("transform", FULL_SCREEN_TRANSFORM);
    shader.setInt("colorAndDepth", 0);
    shader.setInt("normalAndSpecularity", 1);
    shader.setVec3f("cameraPosition", camera.position);
    shader.setMatrix4f("inverseProjection", ctx.inverseProjection);
    shader.setMatrix4f("inverseView", ctx.inverseView);

    for (uint32 i = 0; i < ctx.directionalLights.size(); i++) {
      auto& light = ctx.directionalLights[i];
      std::string indexedLight = "lights[" + std::to_string(i) + "]";

      shader.setVec3f(indexedLight + ".color", light.color);
      shader.setFloat(indexedLight + ".power", light.power);
      shader.setVec3f(indexedLight + ".direction", light.direction);
    }

    OpenGLScreenQuad::render();
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::renderDirectionalShadowcasters() {
    auto& camera = *Camera::active;
    auto& shader = shaders.directionalShadowcaster;

    shader.use();

    for (uint32 i = 0; i < ctx.directionalShadowcasters.size(); i++) {
      auto& light = ctx.directionalShadowcasters[i];
      auto& glShadowMap = *glDirectionalShadowMaps[i];

      glShadowMap.buffer.read();

      shader.setVec4f("transform", FULL_SCREEN_TRANSFORM);
      // @todo define an enum for reserved color attachment indexes
      shader.setInt("colorAndDepth", 0);
      shader.setInt("normalAndSpecularity", 1);
      shader.setInt("shadowMaps[0]", 3);
      shader.setInt("shadowMaps[1]", 4);
      shader.setInt("shadowMaps[2]", 5);
      shader.setMatrix4f("lightMatrices[0]", Gm_CreateCascadedLightViewMatrixGL(0, light.direction, camera));
      shader.setMatrix4f("lightMatrices[1]", Gm_CreateCascadedLightViewMatrixGL(1, light.direction, camera));
      shader.setMatrix4f("lightMatrices[2]", Gm_CreateCascadedLightViewMatrixGL(2, light.direction, camera));
      shader.setVec3f("cameraPosition", camera.position);
      shader.setMatrix4f("inverseProjection", ctx.inverseProjection);
      shader.setMatrix4f("inverseView", ctx.inverseView);
      shader.setVec3f("light.color", light.color);
      shader.setFloat("light.power", light.power);
      shader.setVec3f("light.direction", light.direction);

      OpenGLScreenQuad::render();
    }
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::renderSpotLights() {
    auto& camera = *Camera::active;
    auto& shader = shaders.spotLight;

    shader.use();
    shader.setInt("colorAndDepth", 0);
    shader.setInt("normalAndSpecularity", 1);
    shader.setVec3f("cameraPosition", camera.position);
    shader.setMatrix4f("inverseProjection", ctx.inverseProjection);
    shader.setMatrix4f("inverseView", ctx.inverseView);

    lightDisc.draw(ctx.spotLights);
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::renderSpotShadowcasters() {
    auto& camera = *Camera::active;
    auto& shader = shaders.spotShadowcaster;

    shader.use();
    shader.setInt("colorAndDepth", 0);
    shader.setInt("normalAndSpecularity", 1);
    shader.setInt("shadowMap", 3);
    shader.setVec3f("cameraPosition", camera.position);
    shader.setMatrix4f("inverseProjection", ctx.inverseProjection);
    shader.setMatrix4f("inverseView", ctx.inverseView);
    shader.setFloat("time", AbstractScene::active->getRunningTime());

    for (uint32 i = 0; i < ctx.spotShadowcasters.size(); i++) {
      auto& glShadowMap = *glSpotShadowMaps[i];
      auto& light = ctx.spotShadowcasters[i];

      Matrix4f lightProjection = Matrix4f::glPerspective({ 1024, 1024 }, 120.0f, 1.0f, light.radius);
      Matrix4f lightView = Matrix4f::lookAt(light.position.gl(), light.direction.invert().gl(), Vec3f(0.0f, 1.0f, 0.0f));
      Matrix4f lightMatrix = (lightProjection * lightView).transpose();

      shader.setMatrix4f("lightMatrix", lightMatrix);

      glShadowMap.buffer.read();
      lightDisc.draw(light);
    }
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::copyEmissiveObjects() {
    // Only copy the color/depth frame where emissive
    // objects have been drawn into the G-Buffer
    glStencilFunc(GL_EQUAL, MeshType::EMISSIVE, 0xFF);

    auto& shader = shaders.copyFrame;

    shader.use();
    shader.setVec4f("transform", FULL_SCREEN_TRANSFORM);
    shader.setInt("colorAndDepth", 0);

    OpenGLScreenQuad::render();

    // Restore the lighting stencil function, since we
    // may render indirect lighting after this
    glStencilFunc(GL_LESS, MeshType::PARTICLE_SYSTEM, 0xFF);
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::renderIndirectLight() {
    // @todo rewrite this a bit less awkwardly
    #define WRAP(index) (index) < 0 ? 3 + (index) : index

    auto& currentIndirectLightBuffer = buffers.indirectLight[frame % 3];
    auto& indirectLightBufferT1 = buffers.indirectLight[WRAP(int(frame) % 3 - 1)];
    auto& indirectLightBufferT2 = buffers.indirectLight[WRAP(int(frame) % 3 - 2)];

    if (
      Gm_IsFlagEnabled(GammaFlags::RENDER_AMBIENT_OCCLUSION) ||
      Gm_IsFlagEnabled(GammaFlags::RENDER_GLOBAL_ILLUMINATION)
    ) {
      buffers.gBuffer.read();
      ctx.accumulationTarget->read();

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 6);
      glGenerateMipmap(GL_TEXTURE_2D);

      indirectLightBufferT1.read();
      indirectLightBufferT2.read(1);
      currentIndirectLightBuffer.write();

      glClear(GL_COLOR_BUFFER_BIT);

      shaders.indirectLight.use();

      #if GAMMA_DEVELOPER_MODE
        Vec2f screenSize((float)internalResolution.width, (float)internalResolution.height);

        shaders.indirectLight.setVec2f("screenSize", screenSize);
      #endif

      shaders.indirectLight.setVec4f("transform", FULL_SCREEN_TRANSFORM);
      shaders.indirectLight.setInt("colorAndDepth", 0);
      shaders.indirectLight.setInt("normalAndSpecularity", 1);
      shaders.indirectLight.setInt("indirectLightT1", 2);
      shaders.indirectLight.setInt("indirectLightT2", 3);
      shaders.indirectLight.setVec3f("cameraPosition", Camera::active->position);
      shaders.indirectLight.setMatrix4f("projection", ctx.projection);
      shaders.indirectLight.setMatrix4f("view", ctx.view);
      shaders.indirectLight.setMatrix4f("inverseProjection", ctx.inverseProjection);
      shaders.indirectLight.setMatrix4f("inverseView", ctx.inverseView);
      shaders.indirectLight.setMatrix4f("viewT1", ctx.previousViews[0]);
      shaders.indirectLight.setMatrix4f("viewT2", ctx.previousViews[1]);
      shaders.indirectLight.setFloat("time", AbstractScene::active->getRunningTime());

      OpenGLScreenQuad::render();

      ctx.accumulationTarget->read();
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    }

    buffers.gBuffer.read();
    currentIndirectLightBuffer.read();
    ctx.accumulationTarget->write();

    shaders.indirectLightComposite.use();

    #if GAMMA_DEVELOPER_MODE
      Vec2f screenSize((float)internalResolution.width, (float)internalResolution.height);

      shaders.indirectLightComposite.setVec2f("screenSize", screenSize);
    #endif

    shaders.indirectLightComposite.setVec4f("transform", FULL_SCREEN_TRANSFORM);
    shaders.indirectLightComposite.setInt("colorAndDepth", 0);
    shaders.indirectLightComposite.setInt("normalAndSpecularity", 1);
    shaders.indirectLightComposite.setInt("indirectLight", 2);

    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE);

    OpenGLScreenQuad::render();

    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::renderSkybox() {
    glStencilFunc(GL_EQUAL, MeshType::SKYBOX, 0xFF);

    auto& camera = *Camera::active;

    shaders.skybox.use();
    shaders.skybox.setVec4f("transform", FULL_SCREEN_TRANSFORM);
    shaders.skybox.setVec3f("cameraPosition", camera.position);
    shaders.skybox.setMatrix4f("inverseProjection", ctx.inverseProjection);
    shaders.skybox.setMatrix4f("inverseView", ctx.inverseView);

    OpenGLScreenQuad::render();
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::renderParticleSystems() {
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glStencilFunc(GL_ALWAYS, MeshType::PARTICLE_SYSTEM, 0xFF);
    glStencilMask(0xFF);

    shaders.particles.use();
    shaders.particles.setMatrix4f("projection", ctx.projection);
    shaders.particles.setMatrix4f("view", ctx.view);
    shaders.particles.setFloat("time", AbstractScene::active->getRunningTime());

    for (auto& glMesh : glMeshes) {
      if (glMesh->isMeshType(MeshType::PARTICLE_SYSTEM)) {
        auto& particles = glMesh->getSourceMesh()->particleSystem;

        // @optimize it would be preferable to use a UBO for particle systems,
        // and simply set the particle system ID uniform here. we're doing
        // too many uniform updates as things currently stand.

        // Set particle system parameters
        shaders.particles.setInt("particles.total", glMesh->getObjectCount());
        shaders.particles.setVec3f("particles.spawn", particles.spawn);
        shaders.particles.setFloat("particles.spread", particles.spread);
        shaders.particles.setFloat("particles.minimum_radius", particles.minimumRadius);
        shaders.particles.setFloat("particles.median_speed", particles.medianSpeed);
        shaders.particles.setFloat("particles.speed_variation", particles.speedVariation);
        shaders.particles.setFloat("particles.median_size", particles.medianSize);
        shaders.particles.setFloat("particles.size_variation", particles.sizeVariation);
        shaders.particles.setFloat("particles.deviation", particles.deviation);

        // Set particle path parameters
        constexpr static uint32 MAX_PATH_POINTS = 10;
        uint32 totalPathPoints = std::min((uint32)particles.path.size(), (uint32)MAX_PATH_POINTS);

        for (uint8 i = 0; i < totalPathPoints; i++) {
          shaders.particles.setVec3f("path.points[" + std::to_string(i) + "]", particles.path[i]);
        }

        shaders.particles.setInt("path.total", totalPathPoints);
        shaders.particles.setBool("path.is_circuit", particles.isCircuit);

        glMesh->render(ctx.primitiveMode);
      }
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glStencilMask(MeshType::SKYBOX);
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::renderReflections() {
    if (
      ctx.hasRefractiveObjects &&
      Gm_IsFlagEnabled(GammaFlags::RENDER_REFRACTIVE_GEOMETRY) &&
      Gm_IsFlagEnabled(GammaFlags::RENDER_REFRACTIVE_GEOMETRY_WITHIN_REFLECTIONS)
    ) {
      // @todo fix + explain this
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_CULL_FACE);

      glStencilFunc(GL_NOTEQUAL, MeshType::REFLECTIVE, 0xFF);
      glStencilMask(MeshType::REFRACTIVE);

      shaders.refractivePrepass.use();

      #if GAMMA_DEVELOPER_MODE
        Vec2f screenSize((float)internalResolution.width, (float)internalResolution.height);

        shaders.refractivePrepass.setVec2f("screenSize", screenSize);
      #endif

      shaders.refractivePrepass.setInt("color_and_depth", 0);
      shaders.refractivePrepass.setMatrix4f("projection", ctx.projection);
      shaders.refractivePrepass.setMatrix4f("view", ctx.view);

      for (auto* glMesh : glMeshes) {
        if (glMesh->isMeshType(MeshType::REFRACTIVE)) {
          glMesh->render(ctx.primitiveMode);
        }
      }

      glDisable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);
    }

    auto& camera = *Camera::active;

    buffers.gBuffer.read();
    ctx.accumulationTarget->read();
    buffers.reflections.write();

    // Render reflections (screen-space + skybox)
    //
    // @todo allow controllable reflection parameters
    glStencilFunc(GL_EQUAL, MeshType::REFLECTIVE, 0xFF);

    shaders.reflections.use();
    shaders.reflections.setVec4f("transform", FULL_SCREEN_TRANSFORM);
    shaders.reflections.setInt("colorAndDepth", 0);
    shaders.reflections.setInt("normalAndSpecularity", 1);
    shaders.reflections.setVec3f("cameraPosition", camera.position);
    shaders.reflections.setMatrix4f("view", ctx.view);
    shaders.reflections.setMatrix4f("inverseView", ctx.inverseView);
    shaders.reflections.setMatrix4f("projection", ctx.projection);
    shaders.reflections.setMatrix4f("inverseProjection", ctx.inverseProjection);

    OpenGLScreenQuad::render();

    buffers.reflections.read();
    ctx.accumulationTarget->write();

    shaders.reflectionsDenoise.use();

    #if GAMMA_DEVELOPER_MODE
      Vec2f screenSize((float)internalResolution.width, (float)internalResolution.height);

      shaders.reflectionsDenoise.setVec2f("screenSize", screenSize);
    #endif

    shaders.reflectionsDenoise.setVec4f("transform", FULL_SCREEN_TRANSFORM);
    shaders.reflectionsDenoise.setInt("colorAndDepth", 0);

    OpenGLScreenQuad::render();
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::renderRefractiveGeometry() {
    auto& camera = *Camera::active;

    // Swap buffers so we can temporarily render the
    // refracted geometry to the second accumulation
    // buffer while reading from the first
    swapAccumulationBuffers();

    // At this point, the accumulation source buffer
    // will contain all effects/shading rendered up
    // to this point, which we can read from when
    // rendering refractions. Write to the second
    // accumulation buffer so we can copy that back
    // into the first afterward.
    ctx.accumulationSource->read();
    ctx.accumulationTarget->write();

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glStencilFunc(GL_ALWAYS, MeshType::REFRACTIVE, 0xFF);
    glStencilMask(0xFF);

    shaders.refractiveGeometry.use();

    #if GAMMA_DEVELOPER_MODE
      Vec2f screenSize((float)internalResolution.width, (float)internalResolution.height);

      shaders.refractiveGeometry.setVec2f("screenSize", screenSize);
    #endif

    shaders.refractiveGeometry.setInt("colorAndDepth", 0);
    shaders.refractiveGeometry.setMatrix4f("projection", ctx.projection);
    shaders.refractiveGeometry.setMatrix4f("inverseProjection", ctx.inverseProjection);
    shaders.refractiveGeometry.setMatrix4f("view", ctx.view);
    shaders.refractiveGeometry.setMatrix4f("inverseView", ctx.inverseView);
    shaders.refractiveGeometry.setVec3f("cameraPosition", camera.position);

    for (auto* glMesh : glMeshes) {
      if (glMesh->isMeshType(MeshType::REFRACTIVE)) {
        glMesh->render(ctx.primitiveMode);
      }
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Now that the current target accumulation buffer contains
    // the rendered refractive geometry, swap the buffers so we
    // can write refractions back into the original target
    // accumulation buffer
    swapAccumulationBuffers();

    ctx.accumulationSource->read();
    ctx.accumulationTarget->write();

    glStencilFunc(GL_EQUAL, MeshType::REFRACTIVE, 0xFF);

    shaders.copyFrame.use();
    shaders.copyFrame.setVec4f("transform", FULL_SCREEN_TRANSFORM);
    shaders.copyFrame.setInt("colorAndDepth", 0);

    OpenGLScreenQuad::render();
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::renderPostEffects() {
    swapAccumulationBuffers();

    ctx.accumulationSource->read();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glViewport(0, 0, Window::size.width, Window::size.height);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_STENCIL_TEST);
    glStencilMask(0xFF);

    post.debanding.use();
    post.debanding.setVec4f("transform", FULL_SCREEN_TRANSFORM);

    OpenGLScreenQuad::render();
  }

  /**
   * @todo description
   */
  void OpenGLRenderer::renderDevBuffers() {
    buffers.gBuffer.read();

    shaders.gBufferDev.use();
    shaders.gBufferDev.setInt("colorAndDepth", 0);
    shaders.gBufferDev.setInt("normalAndSpecularity", 1);
    shaders.gBufferDev.setVec4f("transform", { 0.53f, 0.82f, 0.43f, 0.11f });

    OpenGLScreenQuad::render();

    for (uint32 i = 0; i < glDirectionalShadowMaps.size(); i++) {
      float yOffset = 0.52f - float(i) * 0.32f;

      glDirectionalShadowMaps[i]->buffer.read();

      shaders.directionalShadowMapDev.use();
      shaders.directionalShadowMapDev.setInt("cascade0", 3);
      shaders.directionalShadowMapDev.setInt("cascade1", 4);
      shaders.directionalShadowMapDev.setInt("cascade2", 5);
      shaders.directionalShadowMapDev.setVec4f("transform", { 0.695f, yOffset, 0.266f, 0.15f });

      OpenGLScreenQuad::render();
    }

    // @todo point light shadow maps?
    // @todo spot light shadow maps?
  }

  void OpenGLRenderer::createMesh(const Mesh* mesh) {
    glMeshes.push_back(new OpenGLMesh(mesh));

    #if GAMMA_DEVELOPER_MODE
      // @todo move to OpenGLMesh
      uint32 totalVertices = mesh->vertices.size();
      uint32 totalTriangles = mesh->faceElements.size() / 3;

      Console::log("[Gamma] OpenGLMesh created:", totalVertices, "vertices,", totalTriangles, "triangles");
    #endif
  }

  void OpenGLRenderer::createShadowMap(const Light* light) {
    switch (light->type) {
      case LightType::DIRECTIONAL_SHADOWCASTER:
        glDirectionalShadowMaps.push_back(new OpenGLDirectionalShadowMap(light));
        break;
      case LightType::POINT_SHADOWCASTER:
        glPointShadowMaps.push_back(new OpenGLPointShadowMap(light));
        break;
      case LightType::SPOT_SHADOWCASTER:
        glSpotShadowMaps.push_back(new OpenGLSpotShadowMap(light));
        break;
    }
  }

  void OpenGLRenderer::destroyMesh(const Mesh* mesh) {
    // @todo
    Console::log("[Gamma] Mesh destroyed!");
  }

  void OpenGLRenderer::destroyShadowMap(const Light* light) {
    // @todo
    Console::log("[Gamma] Shadowcaster destroyed!");
  }

  const RenderStats& OpenGLRenderer::getRenderStats() {
    GLint total = 0;
    GLint available = 0;
    const char* vendor = (const char*)glGetString(GL_VENDOR);

    if (strcmp(vendor, "NVIDIA Corporation") == 0) {
      glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &total);
      glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &available);
    }

    stats.gpuMemoryTotal = total / 1000;
    stats.gpuMemoryUsed = (total - available) / 1000;
    stats.isVSynced = SDL_GL_GetSwapInterval() == 1;

    return stats;
  }

  void OpenGLRenderer::present() {
    SDL_GL_SwapWindow(sdl_window);
  }

  void OpenGLRenderer::swapAccumulationBuffers() {
    OpenGLFrameBuffer* source = ctx.accumulationSource;

    ctx.accumulationSource = ctx.accumulationTarget;
    ctx.accumulationTarget = source;
  }

  void OpenGLRenderer::renderSurfaceToScreen(SDL_Surface* surface, uint32 x, uint32 y, const Vec3f& color, const Vec4f& background) {
    float offsetX = -1.0f + (2 * x + surface->w) / (float)Window::size.width;
    float offsetY = 1.0f - (2 * y + surface->h) / (float)Window::size.height;
    float scaleX = surface->w / (float)Window::size.width;
    float scaleY = -1.0f * surface->h / (float)Window::size.height;
    int format = surface->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    screen.use();
    screen.setVec4f("transform", { offsetX, offsetY, scaleX, scaleY });
    screen.setVec3f("color", color);
    screen.setVec4f("background", background);

    OpenGLScreenQuad::render();
  }

  void OpenGLRenderer::renderText(TTF_Font* font, const char* message, uint32 x, uint32 y, const Vec3f& color, const Vec4f& background) {
    SDL_Surface* text = TTF_RenderText_Blended(font, message, { 255, 255, 255 });

    renderSurfaceToScreen(text, x, y, color, background);

    SDL_FreeSurface(text);
  }
}