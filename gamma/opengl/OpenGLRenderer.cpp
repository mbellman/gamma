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
    // internalResolution = { 960, 540 };

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
  }

  void OpenGLRenderer::destroy() {
    Gm_DestroyRendererResources(buffers, shaders);
    Gm_DestroyDrawIndirectBuffer();

    lightDisc.destroy();

    glDeleteTextures(1, &screenTexture);

    SDL_GL_DeleteContext(glContext);
  }

  // @todo time to split this up into sub-methods. use a 'render context'
  // to keep track of matrices/lights/etc.
  void OpenGLRenderer::render() {
    if (AbstractScene::active == nullptr) {
      return;
    }
  
    // Setup variables
    uint32 internalWidth = internalResolution.width;
    uint32 internalHeight = internalResolution.height;
    bool hasReflectiveObjects = false;
    bool hasRefractiveObjects = false;
    GLenum primitiveMode = Gm_IsFlagEnabled(GammaFlags::WIREFRAME_MODE) ? GL_LINES : GL_TRIANGLES;

    // Build light arrays by type
    //
    // @todo don't reallocate on every frame
    std::vector<Light> pointLights;
    std::vector<Light> pointShadowCasters;
    std::vector<Light> directionalLights;
    std::vector<Light> directionalShadowcasters;
    std::vector<Light> spotLights;
    std::vector<Light> spotShadowcasters;

    for (auto& light : AbstractScene::active->getLights()) {
      switch (light.type) {
        case LightType::POINT:
          pointLights.push_back(light);
          break;
        case LightType::POINT_SHADOWCASTER:
          if (Gm_IsFlagEnabled(GammaFlags::RENDER_SHADOWS)) {
            pointShadowCasters.push_back(light);
          } else {
            pointLights.push_back(light);
          }

          break;
        case LightType::DIRECTIONAL:
          directionalLights.push_back(light);
          break;
        case LightType::DIRECTIONAL_SHADOWCASTER:
          if (Gm_IsFlagEnabled(GammaFlags::RENDER_SHADOWS)) {
            directionalShadowcasters.push_back(light);
          } else {
            directionalLights.push_back(light);
          }

          break;
        case LightType::SPOT:
          spotLights.push_back(light);
          break;
        case LightType::SPOT_SHADOWCASTER:
          if (Gm_IsFlagEnabled(GammaFlags::RENDER_SHADOWS)) {
            spotShadowcasters.push_back(light);
          } else {
            spotLights.push_back(light);
          }

          break;
      }
    }

    // Check to see if reflective/refractive geometry passes are necessary
    for (auto* glMesh : glMeshes) {
      if (glMesh->getObjectCount() > 0) {
        if (glMesh->isMeshType(MeshType::REFLECTIVE)) {
          hasReflectiveObjects = true;
        } else if (glMesh->isMeshType(MeshType::REFRACTIVE)) {
          hasRefractiveObjects = true;
        }
      }
    }

    // Handle vsync setting changes
    if (Gm_IsFlagEnabled(GammaFlags::VSYNC) && SDL_GL_GetSwapInterval() == 0) {
      SDL_GL_SetSwapInterval(1);

      #if GAMMA_DEVELOPER_MODE
        Console::log("[Gamma] V-Sync enabled");
      #endif
    } else if (!Gm_IsFlagEnabled(GammaFlags::VSYNC) && SDL_GL_GetSwapInterval() == 1) {
      SDL_GL_SetSwapInterval(0);

      #if GAMMA_DEVELOPER_MODE
        Console::log("[Gamma] V-Sync disabled");
      #endif
    }

    // Set G-Buffer as render target
    buffers.gBuffer.write();

    // Clear buffers, reset state
    glViewport(0, 0, internalWidth, internalHeight);
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

    // Render camera view
    auto& camera = *Camera::active;
    Matrix4f projection = Matrix4f::glPerspective({ internalWidth, internalHeight }, 45.0f, 1.0f, 10000.0f).transpose();

    Matrix4f view = (
      Matrix4f::rotation(camera.orientation.toVec3f()) *
      Matrix4f::translation(camera.position.invert().gl())
    ).transpose();

    shaders.geometry.use();
    shaders.geometry.setMatrix4f("projection", projection);
    shaders.geometry.setMatrix4f("view", view);
    shaders.geometry.setInt("meshTexture", 0);
    shaders.geometry.setInt("meshNormalMap", 1);

    // @todo render emissive objects
    // glStencilMask(MeshType::EMISSIVE);

    // Render reflective objects
    glStencilMask(MeshType::REFLECTIVE);

    for (auto* glMesh : glMeshes) {
      if (glMesh->isMeshType(MeshType::REFLECTIVE)) {
        shaders.geometry.setBool("hasTexture", glMesh->hasTexture());
        shaders.geometry.setBool("hasNormalMap", glMesh->hasNormalMap());

        glMesh->render(primitiveMode);
      }
    }

    // Render non-emissive, non-reflective objects
    glStencilMask(MeshType::NON_EMISSIVE);

    for (auto* glMesh : glMeshes) {
      if (glMesh->isMeshType(MeshType::NON_EMISSIVE)) {
        shaders.geometry.setBool("hasTexture", glMesh->hasTexture());
        shaders.geometry.setBool("hasNormalMap", glMesh->hasNormalMap());

        glMesh->render(primitiveMode);
      }
    }

    // Render directional shadow maps
    if (glDirectionalShadowMaps.size() > 0 && Gm_IsFlagEnabled(GammaFlags::RENDER_SHADOWS)) {
      glDisable(GL_STENCIL_TEST);

      auto& shader = shaders.directionalShadowcasterView;

      shader.use();

      for (uint32 mapIndex = 0; mapIndex < glDirectionalShadowMaps.size(); mapIndex++) {
        auto& glShadowMap = *glDirectionalShadowMaps[mapIndex];
        auto& light = directionalShadowcasters[mapIndex];

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
              glMesh->render(primitiveMode, true);
            }
          }
        }
      }

      glEnable(GL_STENCIL_TEST);
    }

    // Render point shadow maps
    if (glPointShadowMaps.size() > 0 && Gm_IsFlagEnabled(GammaFlags::RENDER_SHADOWS)) {
      glDisable(GL_STENCIL_TEST);

      Vec3f directions[6] = {
        Vec3f(-1.0f, 0.0f, 0.0f),
        Vec3f(1.0f, 0.0f, 0.0f),
        Vec3f(0.0f, -1.0f, 0.0f),
        Vec3f(0.0f, 1.0f, 0.0f),
        Vec3f(0.0f, 0.0f, -1.0f),
        Vec3f(0.0f, 0.0f, 1.0f)
      };

      Vec3f tops[6] = {
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
        auto& light = pointShadowCasters[mapIndex];

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
            glMesh->render(primitiveMode, true);
          }
        }

        glShadowMap.isRendered = true;
      }

      glEnable(GL_STENCIL_TEST);
    }

    if (glSpotShadowMaps.size() > 0 && Gm_IsFlagEnabled(GammaFlags::RENDER_SHADOWS)) {
      // @todo
    }

    // Lighting pass; read from G-Buffer and preemptively write to post buffer
    buffers.gBuffer.read();
    buffers.post.write();

    glViewport(0, 0, internalWidth, internalHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glStencilFunc(GL_NOTEQUAL, MeshType::EMISSIVE, 0xFF);
    glStencilMask(0x00);

    Matrix4f inverseProjection = projection.inverse();
    Matrix4f inverseView = view.inverse();

    // If no directional lights/shadowcasters are available,
    // perform a screen pass to copy depth information for
    // all on-screen geometry into the post buffer. During
    // the reflection and refractive geometry passes, we
    // copy the post buffer back into G-Buffer attachment 2
    // so that accumulated effects and normals can continue
    // to be read when writing to the post buffer again. If
    // we don't do this, reflections and refractions will
    // miss any surfaces which aren't directly illuminated,
    // i.e. otherwise written to the post buffer in the
    // directional lighting passes.
    if (directionalLights.size() == 0 && directionalShadowcasters.size() == 0) {
      auto& shader = shaders.copyDepth;

      shader.use();
      shader.setVec4f("transform", FULL_SCREEN_TRANSFORM);
      shader.setInt("color_and_depth", 0);

      OpenGLScreenQuad::render();
    }

    // Render point lights (non-shadowcasters)
    if (pointLights.size() > 0) {
      auto& shader = shaders.pointLight;

      shader.use();
      shader.setInt("colorAndDepth", 0);
      shader.setInt("normalAndSpecularity", 1);
      shader.setVec3f("cameraPosition", camera.position);
      shader.setMatrix4f("inverseProjection", inverseProjection);
      shader.setMatrix4f("inverseView", inverseView);

      lightDisc.draw(pointLights);
    }

    // Render point shadowcaster lights
    if (pointShadowCasters.size() > 0) {
      auto& shader = shaders.pointShadowcaster;

      shader.use();
      shader.setInt("colorAndDepth", 0);
      shader.setInt("normalAndSpecularity", 1);
      shader.setInt("shadowMap", 3);
      shader.setVec3f("cameraPosition", camera.position);
      shader.setMatrix4f("inverseProjection", inverseProjection);
      shader.setMatrix4f("inverseView", inverseView);

      for (uint32 i = 0; i < pointShadowCasters.size(); i++) {
        auto& glShadowMap = *glPointShadowMaps[i];
        auto& light = pointShadowCasters[i];

        glShadowMap.buffer.read();
        lightDisc.draw(light);
      }
    }

    // Render directional lights (non-shadowcasters)
    if (directionalLights.size() > 0) {
      auto& shader = shaders.directionalLight;

      shader.use();
      shader.setVec4f("transform", FULL_SCREEN_TRANSFORM);
      shader.setInt("colorAndDepth", 0);
      shader.setInt("normalAndSpecularity", 1);
      shader.setVec3f("cameraPosition", camera.position);
      shader.setMatrix4f("inverseProjection", inverseProjection);
      shader.setMatrix4f("inverseView", inverseView);

      for (uint32 i = 0; i < directionalLights.size(); i++) {
        auto& light = directionalLights[i];
        std::string indexedLight = "lights[" + std::to_string(i) + "]";

        shader.setVec3f(indexedLight + ".color", light.color);
        shader.setFloat(indexedLight + ".power", light.power);
        shader.setVec3f(indexedLight + ".direction", light.direction);
      }

      OpenGLScreenQuad::render();
    }

    // Render directional shadowcaster lights
    if (directionalShadowcasters.size() > 0) {
      buffers.gBuffer.read();
      buffers.post.write();

      auto& shader = shaders.directionalShadowcaster;

      shader.use();

      for (uint32 i = 0; i < directionalShadowcasters.size(); i++) {
        auto& light = directionalShadowcasters[i];
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
        shader.setMatrix4f("inverseProjection", inverseProjection);
        shader.setMatrix4f("inverseView", inverseView);
        shader.setVec3f("light.color", light.color);
        shader.setFloat("light.power", light.power);
        shader.setVec3f("light.direction", light.direction);

        OpenGLScreenQuad::render();
      }
    }

    if (spotLights.size() > 0) {
      auto& shader = shaders.spotLight;

      shader.use();
      shader.setInt("colorAndDepth", 0);
      shader.setInt("normalAndSpecularity", 1);
      shader.setVec3f("cameraPosition", camera.position);
      shader.setMatrix4f("inverseProjection", inverseProjection);
      shader.setMatrix4f("inverseView", inverseView);

      lightDisc.draw(spotLights);
    }

    if (spotShadowcasters.size() > 0) {
      // @todo
    }

    // Render skybox
    glDisable(GL_BLEND);
    glStencilFunc(GL_EQUAL, MeshType::EMISSIVE, 0xFF);

    shaders.skybox.use();
    shaders.skybox.setVec4f("transform", FULL_SCREEN_TRANSFORM);
    shaders.skybox.setVec3f("cameraPosition", camera.position);
    shaders.skybox.setMatrix4f("inverseProjection", inverseProjection);
    shaders.skybox.setMatrix4f("inverseView", inverseView);

    OpenGLScreenQuad::render();

    if (hasReflectiveObjects && Gm_IsFlagEnabled(GammaFlags::RENDER_REFLECTIONS)) {
      // Copy the rendered frame back into the color accumulation
      // channel of the G-Buffer, since reflections rely on both
      // accumulated color/depth and surface normals. We exclude
      // skybox pixels since we recompute skybox color in the
      // reflection shader for any skybound reflection rays.
      //
      // @todo distinguish between skybox and emissive pixels so
      // emissive geometry can still be reflected
      glStencilFunc(GL_NOTEQUAL, MeshType::EMISSIVE, 0xFF);

      writeAccumulatedEffectsBackIntoGBuffer();

      if (
        hasRefractiveObjects &&
        Gm_IsFlagEnabled(GammaFlags::RENDER_REFRACTIVE_OBJECTS) &&
        Gm_IsFlagEnabled(GammaFlags::RENDER_REFRACTIVE_OBJECTS_WITHIN_REFLECTIONS)
      ) {
        // @todo explain this
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        glStencilFunc(GL_NOTEQUAL, MeshType::REFLECTIVE, 0xFF);
        glStencilMask(MeshType::REFRACTIVE);

        shaders.refractivePrepass.use();
        shaders.refractivePrepass.setInt("color_and_depth", 0);
        shaders.refractivePrepass.setMatrix4f("projection", projection);
        shaders.refractivePrepass.setMatrix4f("view", view);

        for (auto* glMesh : glMeshes) {
          if (glMesh->isMeshType(MeshType::REFRACTIVE)) {
            glMesh->render(primitiveMode);
          }
        }

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
      }

      buffers.gBuffer.read();
      buffers.reflections.write();

      // Render reflections (screen-space + skybox)
      //
      // @todo allow controllable reflection parameters
      glStencilFunc(GL_EQUAL, MeshType::REFLECTIVE, 0xFF);
      // glViewport(0, 0, internalWidth / 2, internalHeight / 2);

      shaders.reflections.use();
      shaders.reflections.setVec4f("transform", FULL_SCREEN_TRANSFORM);
      shaders.reflections.setInt("normalAndSpecularity", 1);
      shaders.reflections.setInt("colorAndDepth", 2);
      shaders.reflections.setVec3f("cameraPosition", camera.position);
      shaders.reflections.setMatrix4f("view", view);
      shaders.reflections.setMatrix4f("inverseView", inverseView);
      shaders.reflections.setMatrix4f("projection", projection);
      shaders.reflections.setMatrix4f("inverseProjection", inverseProjection);

      OpenGLScreenQuad::render();
      // glViewport(0, 0, internalWidth, internalHeight);

      buffers.reflections.read();
      buffers.post.write();

      shaders.reflectionsDenoise.use();
      shaders.reflectionsDenoise.setVec4f("transform", FULL_SCREEN_TRANSFORM);
      shaders.reflectionsDenoise.setInt("colorAndDepth", 0);

      OpenGLScreenQuad::render();
    }

    if (hasRefractiveObjects && Gm_IsFlagEnabled(GammaFlags::RENDER_REFRACTIVE_OBJECTS)) {
      // Copy the rendered frame back into the color accumulation channel
      // of the G-Buffer so we can accurately render refractive geometry,
      // which relies on both accumulated color/depth and surface normals.
      if (hasReflectiveObjects && Gm_IsFlagEnabled(GammaFlags::RENDER_REFLECTIONS)) {
        // Only reflective surfaces need to be copied now, since we copied
        // all non-skybox pixels into the color accumulation buffer before
        // rendering reflections.
        glStencilFunc(GL_EQUAL, MeshType::REFLECTIVE, 0xFF);
      } else {
        // If no reflections were rendered, we need to copy all non-skybox
        // pixels back into the color accumulation channel here. The skybox
        // color will be recomputed in the refractive geometry shader for
        // any skybound refraction rays.
        //
        // @todo distinguish between skybox and emissive pixels so
        // emissive geometry can still be refracted
        glStencilFunc(GL_NOTEQUAL, MeshType::EMISSIVE, 0xFF);
      }

      writeAccumulatedEffectsBackIntoGBuffer();

      buffers.gBuffer.read();
      buffers.post.write();

      glEnable(GL_CULL_FACE);
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_STENCIL_TEST);

      shaders.refractiveGeometry.use();
      shaders.refractiveGeometry.setInt("colorAndDepth", 2);
      shaders.refractiveGeometry.setMatrix4f("projection", projection);
      shaders.refractiveGeometry.setMatrix4f("inverseProjection", inverseProjection);
      shaders.refractiveGeometry.setMatrix4f("view", view);
      shaders.refractiveGeometry.setMatrix4f("inverseView", inverseView);
      shaders.refractiveGeometry.setVec3f("cameraPosition", camera.position);

      for (auto* glMesh : glMeshes) {
        if (glMesh->isMeshType(MeshType::REFRACTIVE)) {
          glMesh->render(primitiveMode);
        }
      }

      glDisable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);
    }

    // Post-processing pass
    buffers.post.read();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glViewport(0, 0, Window::size.width, Window::size.height);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_STENCIL_TEST);
    glStencilMask(0xFF);

    post.debanding.use();
    post.debanding.setVec4f("transform", FULL_SCREEN_TRANSFORM);

    OpenGLScreenQuad::render();

    #if GAMMA_DEVELOPER_MODE
      if (Gm_IsFlagEnabled(GammaFlags::SHOW_DEV_BUFFERS)) {
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
      }
    #endif

    frame++;
  }

  void OpenGLRenderer::createMesh(const Mesh* mesh) {
    glMeshes.push_back(new OpenGLMesh(mesh));

    #if GAMMA_DEVELOPER_MODE
      uint32 totalVertices = mesh->vertices.size();
      uint32 totalTriangles = mesh->faceElements.size() / 3;

      Console::log("[Gamma] Mesh created!", totalVertices, "vertices,", totalTriangles, "triangles");
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

    Console::log("[Gamma] Shadow map created!");
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

  /**
   * Copies the accumulated results of the current frame
   * back into the G-Buffer, color attachment 2. Certain
   * effects require normal/specularity information from
   * the G-Buffer, as well as more up-to-date color information
   * from lighting and other secondary effects, before the final
   * post-processing stage. Color attachment 2 is designated
   * as an alternate color buffer containing more than just the
   * raw geometry albedo, which does not represent the final
   * on-screen image.
   */
  void OpenGLRenderer::writeAccumulatedEffectsBackIntoGBuffer() {
    buffers.post.read();
    buffers.gBuffer.write();

    shaders.copyFrame.use();
    shaders.copyFrame.setVec4f("transform", FULL_SCREEN_TRANSFORM);
    shaders.copyFrame.setInt("colorAndDepth", 0);

    OpenGLScreenQuad::render();
  }
}