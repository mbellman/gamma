#include <cstdio>
#include <map>

#include "opengl/errors.h"
#include "opengl/OpenGLRenderer.h"
#include "opengl/OpenGLScreenQuad.h"
#include "opengl/renderer_helpers.h"
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
    flags = OpenGLRenderFlags::RENDER_DEFERRED;
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

    // Initialize font texture
    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Initialize forward renderer
    // @todo

    // Initialize dynamic lights UBO
    // @todo buffer lights to forward renderer geometry shader
    // glGenBuffers(1, &forward.lightsUbo);
    // glBindBuffer(GL_UNIFORM_BUFFER, forward.lightsUbo);
    // glBufferData(GL_UNIFORM_BUFFER, MAX_LIGHTS * sizeof(Light), 0, GL_DYNAMIC_DRAW);
    // glBindBufferBase(GL_UNIFORM_BUFFER, 0, forward.lightsUbo);
    // glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Initialize deferred renderer
    // @todo define separate OpenGLDeferredRenderer/OpenGLForwardRenderer classes
    Gm_InitDeferredRenderPath(deferred, internalResolution);

    deferred.lightDisc.init();

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

  void OpenGLRenderer::render() {
    // @todo define separate OpenGLDeferredRenderer/OpenGLForwardRenderer classes
    if (flags & OpenGLRenderFlags::RENDER_DEFERRED) {
      renderDeferred();
    } else {
      renderForward();
    }
  }

  void OpenGLRenderer::destroy() {
    Gm_DestroyDeferredRenderPath(deferred);

    // glDeleteBuffers(1, &forward.lightsUbo);
    glDeleteTextures(1, &screenTexture);

    SDL_GL_DeleteContext(glContext);
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

    Console::log("[Gamma] Shadowcaster created!");
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

  void OpenGLRenderer::renderDeferred() {
    if (AbstractScene::active == nullptr) {
      return;
    }
  
    uint32 internalWidth = internalResolution.width;
    uint32 internalHeight = internalResolution.height;
    bool hasReflectiveObjects = false;
    bool hasRefractiveObjects = false;

    // Build categorized light arrays
    //
    // @todo don't reallocate on every frame
    auto& lights = AbstractScene::active->getLights();
    std::vector<Light> pointLights;
    std::vector<Light> directionalLights;
    std::vector<Light> directionalShadowcasters;

    for (auto& light : lights) {
      switch (light.type) {
        case LightType::POINT:
          pointLights.push_back(light);
          break;
        case LightType::DIRECTIONAL:
          directionalLights.push_back(light);
          break;
        case LightType::DIRECTIONAL_SHADOWCASTER:
          if (Gm_GetFlags() & GammaFlags::RENDER_SHADOWS) {
            directionalShadowcasters.push_back(light);
          } else {
            directionalLights.push_back(light);
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
    if (Gm_GetFlags() & GammaFlags::VSYNC && SDL_GL_GetSwapInterval() == 0) {
      SDL_GL_SetSwapInterval(1);

      #if GAMMA_DEVELOPER_MODE
        Console::log("[Gamma] V-Sync enabled");
      #endif
    } else if (!(Gm_GetFlags() & GammaFlags::VSYNC) && SDL_GL_GetSwapInterval() == 1) {
      SDL_GL_SetSwapInterval(0);

      #if GAMMA_DEVELOPER_MODE
        Console::log("[Gamma] V-Sync disabled");
      #endif
    }

    // Set G-Buffer as render target
    deferred.buffers.gBuffer.write();

    // Clear buffers, reset state
    glViewport(0, 0, internalWidth, internalHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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

    deferred.shaders.geometry.use();
    deferred.shaders.geometry.setMatrix4f("projection", projection);
    deferred.shaders.geometry.setMatrix4f("view", view);
    deferred.shaders.geometry.setInt("meshTexture", 0);
    deferred.shaders.geometry.setInt("meshNormalMap", 1);

    GLenum primitiveMode = Gm_GetFlags() & GammaFlags::WIREFRAME_MODE
      ? GL_LINE_STRIP
      : GL_TRIANGLES;

    // @todo render emissive objects
    // glStencilMask(MeshType::EMISSIVE);

    // Render reflective objects
    glStencilMask(MeshType::REFLECTIVE);

    for (auto* glMesh : glMeshes) {
      if (glMesh->isMeshType(MeshType::REFLECTIVE)) {
        deferred.shaders.geometry.setBool("hasTexture", glMesh->hasTexture());
        deferred.shaders.geometry.setBool("hasNormalMap", glMesh->hasNormalMap());

        glMesh->render(primitiveMode);
      }
    }

    // Render non-emissive, non-reflective objects
    glStencilMask(MeshType::NON_EMISSIVE);

    for (auto* glMesh : glMeshes) {
      if (glMesh->isMeshType(MeshType::NON_EMISSIVE)) {
        deferred.shaders.geometry.setBool("hasTexture", glMesh->hasTexture());
        deferred.shaders.geometry.setBool("hasNormalMap", glMesh->hasNormalMap());

        glMesh->render(primitiveMode);
      }
    }

    // Render directional shadow maps
    if (glDirectionalShadowMaps.size() > 0 && Gm_GetFlags() & GammaFlags::RENDER_SHADOWS) {
      glDisable(GL_STENCIL_TEST);

      auto& shader = deferred.shaders.directionalShadowcasterView;

      shader.use();

      for (uint32 mapIndex = 0; mapIndex < glDirectionalShadowMaps.size(); mapIndex++) {
        auto& glShadowMap = *glDirectionalShadowMaps[mapIndex];

        glShadowMap.buffer.write();

        for (uint32 cascade = 0; cascade < 3; cascade++) {
          glShadowMap.buffer.writeToAttachment(cascade);
          Matrix4f lightView = Gm_CreateCascadedLightViewMatrixGL(cascade, directionalShadowcasters[mapIndex].direction, camera);

          shader.setMatrix4f("lightView", lightView);

          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

          // @todo glMultiDrawElementsIndirect for static world geometry
          // (will require a handful of other changes to mesh organization/data buffering)
          for (auto* glMesh : glMeshes) {
            auto* sourceMesh = glMesh->getSourceMesh();

            if (sourceMesh->canCastShadows && sourceMesh->maxCascade >= cascade) {
              glMesh->render(primitiveMode);
            }
          }
        }
      }

      glEnable(GL_STENCIL_TEST);
    }

    // Lighting pass; read from G-Buffer and preemptively write to post buffer
    deferred.buffers.gBuffer.read();
    deferred.buffers.post.write();

    glViewport(0, 0, internalWidth, internalHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);
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
      auto& shader = deferred.shaders.copyDepth;

      shader.use();
      shader.setVec4f("transform", FULL_SCREEN_TRANSFORM);
      shader.setInt("color_and_depth", 0);

      OpenGLScreenQuad::render();
    }

    // Render point lights (non-shadowcasters)
    if (pointLights.size() > 0) {
      auto& shader = deferred.shaders.pointLight;

      shader.use();
      shader.setInt("colorAndDepth", 0);
      shader.setInt("normalAndSpecularity", 1);
      shader.setVec3f("cameraPosition", camera.position);
      shader.setMatrix4f("inverseProjection", inverseProjection);
      shader.setMatrix4f("inverseView", inverseView);

      deferred.lightDisc.draw(pointLights);
    }

    // Render directional lights (non-shadowcasters)
    if (directionalLights.size() > 0) {
      auto& shader = deferred.shaders.directionalLight;

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
      deferred.buffers.gBuffer.read();
      deferred.buffers.post.write();

      auto& shader = deferred.shaders.directionalShadowcaster;

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

    // @todo additional shadowcaster light passes

    // Render skybox
    glDisable(GL_BLEND);
    glStencilFunc(GL_EQUAL, MeshType::EMISSIVE, 0xFF);

    deferred.shaders.skybox.use();
    deferred.shaders.skybox.setVec4f("transform", FULL_SCREEN_TRANSFORM);
    deferred.shaders.skybox.setVec3f("cameraPosition", camera.position);
    deferred.shaders.skybox.setMatrix4f("inverseProjection", inverseProjection);
    deferred.shaders.skybox.setMatrix4f("inverseView", inverseView);

    OpenGLScreenQuad::render();

    if (hasReflectiveObjects && (Gm_GetFlags() & GammaFlags::RENDER_REFLECTIONS)) {
      // Write all non-skybox pixels back into the accumulated
      // color buffer. If the sky is reflected on a surface, its
      // color is computed in the reflections shader rather than
      // being read from the color buffer.
      //
      // @todo distinguish between skybox and emissive pixels so
      // emissive geometry can still be reflected
      glStencilFunc(GL_NOTEQUAL, MeshType::EMISSIVE, 0xFF);

      writeAccumulatedEffectsBackIntoGBuffer();

      deferred.buffers.gBuffer.read();
      deferred.buffers.reflections.write();

      // Render reflections (screen-space + skybox)
      //
      // @todo allow controllable reflection parameters
      glStencilFunc(GL_EQUAL, MeshType::REFLECTIVE, 0xFF);
      // glViewport(0, 0, internalWidth / 2, internalHeight / 2);

      deferred.shaders.reflections.use();
      deferred.shaders.reflections.setVec4f("transform", FULL_SCREEN_TRANSFORM);
      deferred.shaders.reflections.setInt("normalAndSpecularity", 1);
      deferred.shaders.reflections.setInt("colorAndDepth", 2);
      deferred.shaders.reflections.setVec3f("cameraPosition", camera.position);
      deferred.shaders.reflections.setMatrix4f("view", view);
      deferred.shaders.reflections.setMatrix4f("inverseView", inverseView);
      deferred.shaders.reflections.setMatrix4f("projection", projection);
      deferred.shaders.reflections.setMatrix4f("inverseProjection", inverseProjection);

      OpenGLScreenQuad::render();
      // glViewport(0, 0, internalWidth, internalHeight);

      deferred.buffers.reflections.read();
      deferred.buffers.post.write();

      deferred.shaders.reflectionsDenoise.use();
      deferred.shaders.reflectionsDenoise.setVec4f("transform", FULL_SCREEN_TRANSFORM);
      deferred.shaders.reflectionsDenoise.setInt("colorAndDepth", 0);

      OpenGLScreenQuad::render();
    }

    if (hasRefractiveObjects && (Gm_GetFlags() & GammaFlags::RENDER_REFRACTIONS)) {
      // Write all non-skybox pixels back into the accumulated
      // color buffer. If refraction rays point toward the sky,
      // we compute the sky color in the shader, rather than
      // reading from the color buffer.
      //
      // @todo distinguish between skybox and emissive pixels so
      // emissive geometry can still be refracted
      glStencilFunc(GL_NOTEQUAL, MeshType::EMISSIVE, 0xFF);

      writeAccumulatedEffectsBackIntoGBuffer();

      deferred.buffers.gBuffer.read();
      deferred.buffers.post.write();

      glEnable(GL_CULL_FACE);
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_STENCIL_TEST);

      deferred.shaders.refractiveGeometry.use();
      deferred.shaders.refractiveGeometry.setInt("colorAndDepth", 2);
      deferred.shaders.refractiveGeometry.setMatrix4f("projection", projection);
      deferred.shaders.refractiveGeometry.setMatrix4f("inverseProjection", inverseProjection);
      deferred.shaders.refractiveGeometry.setMatrix4f("view", view);
      deferred.shaders.refractiveGeometry.setMatrix4f("inverseView", inverseView);
      deferred.shaders.refractiveGeometry.setVec3f("cameraPosition", camera.position);

      for (auto* glMesh : glMeshes) {
        if (glMesh->isMeshType(MeshType::REFRACTIVE)) {
          glMesh->render(primitiveMode);
        }
      }

      glDisable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);
    }

    // Post-processing pass
    deferred.buffers.post.read();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glViewport(0, 0, Window::size.width, Window::size.height);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_STENCIL_TEST);
    glStencilMask(0xFF);

    post.debanding.use();
    post.debanding.setVec4f("transform", FULL_SCREEN_TRANSFORM);

    OpenGLScreenQuad::render();

    #if GAMMA_DEVELOPER_MODE
      if (Gm_GetFlags() & GammaFlags::SHOW_DEBUG_BUFFERS) {
        deferred.buffers.gBuffer.read();

        deferred.shaders.gBufferDebug.use();
        deferred.shaders.gBufferDebug.setInt("colorAndDepth", 0);
        deferred.shaders.gBufferDebug.setInt("normalAndSpecularity", 1);
        deferred.shaders.gBufferDebug.setVec4f("transform", { 0.53f, 0.82f, 0.43f, 0.11f });

        OpenGLScreenQuad::render();

        for (uint32 i = 0; i < glDirectionalShadowMaps.size(); i++) {
          float yOffset = 0.52f - float(i) * 0.32f;

          glDirectionalShadowMaps[i]->buffer.read();

          deferred.shaders.directionalShadowMapDebug.use();
          deferred.shaders.directionalShadowMapDebug.setInt("cascade0", 3);
          deferred.shaders.directionalShadowMapDebug.setInt("cascade1", 4);
          deferred.shaders.directionalShadowMapDebug.setInt("cascade2", 5);
          deferred.shaders.directionalShadowMapDebug.setVec4f("transform", { 0.695f, yOffset, 0.266f, 0.15f });

          OpenGLScreenQuad::render();
        }
      }
    #endif

    frame++;
  }

  void OpenGLRenderer::renderForward() {
    // @todo
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
    deferred.buffers.post.read();
    deferred.buffers.gBuffer.write();

    deferred.shaders.copyFrame.use();
    deferred.shaders.copyFrame.setVec4f("transform", FULL_SCREEN_TRANSFORM);
    deferred.shaders.copyFrame.setInt("colorAndDepth", 0);

    OpenGLScreenQuad::render();
  }
}