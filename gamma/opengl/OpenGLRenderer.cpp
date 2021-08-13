#include <cstdio>
#include <map>

#include "opengl/errors.h"
#include "opengl/OpenGLRenderer.h"
#include "opengl/OpenGLScreenQuad.h"
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

  const enum StencilType {
    EMISSIVE_OBJECTS = 0x00,
    REFLECTIVE_OBJECTS = 0xF0,
    NON_EMISSIVE_OBJECTS = 0xFF
  };

  /**
   * OpenGLRenderer
   * --------------
   */
  void OpenGLRenderer::init() {
    flags = OpenGLRenderFlags::RENDER_DEFERRED | OpenGLRenderFlags::RENDER_SHADOWS;
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
    glGenBuffers(1, &forward.lightsUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, forward.lightsUbo);
    glBufferData(GL_UNIFORM_BUFFER, MAX_LIGHTS * sizeof(Light), 0, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, forward.lightsUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Initialize deferred renderer
    // @todo define separate OpenGLDeferredRenderer/OpenGLForwardRenderer classes
    deferred.g_buffer.init();
    deferred.g_buffer.setSize(internalResolution);
    deferred.g_buffer.addColorAttachment(ColorFormat::RGBA);    // (RGB) Color, (A) Depth
    deferred.g_buffer.addColorAttachment(ColorFormat::RGBA16);  // (RGB) Normal, (A) Specularity
    deferred.g_buffer.addColorAttachment(ColorFormat::RGBA);    // (RGB) Color, (A) Depth - Previous frame
    deferred.g_buffer.addDepthStencilAttachment();
    deferred.g_buffer.bindColorAttachments();

    deferred.post_buffer.init();
    deferred.post_buffer.setSize(internalResolution);
    deferred.post_buffer.addColorAttachment(ColorFormat::RGBA);  // (RGB) Color, (A) Depth
    deferred.g_buffer.shareDepthStencilAttachment(deferred.post_buffer);
    deferred.post_buffer.bindColorAttachments();

    deferred.copyFrame.init();
    deferred.copyFrame.attachShader(Gm_CompileVertexShader("./gamma/opengl/shaders/quad.vert.glsl"));
    deferred.copyFrame.attachShader(Gm_CompileFragmentShader("./gamma/opengl/shaders/deferred/copy-frame.frag.glsl"));
    deferred.copyFrame.link();

    deferred.geometry.init();
    deferred.geometry.attachShader(Gm_CompileVertexShader("./gamma/opengl/shaders/deferred/geometry.vert.glsl"));
    deferred.geometry.attachShader(Gm_CompileFragmentShader("./gamma/opengl/shaders/deferred/geometry.frag.glsl"));
    deferred.geometry.link();

    deferred.pointLightWithoutShadow.init();
    deferred.pointLightWithoutShadow.attachShader(Gm_CompileVertexShader("./gamma/opengl/shaders/deferred/instanced-light.vert.glsl"));
    deferred.pointLightWithoutShadow.attachShader(Gm_CompileFragmentShader("./gamma/opengl/shaders/deferred/point-light-without-shadow.frag.glsl"));
    deferred.pointLightWithoutShadow.link();

    deferred.directionalLightWithoutShadow.init();
    deferred.directionalLightWithoutShadow.attachShader(Gm_CompileVertexShader("./gamma/opengl/shaders/quad.vert.glsl"));
    deferred.directionalLightWithoutShadow.attachShader(Gm_CompileFragmentShader("./gamma/opengl/shaders/deferred/directional-light-without-shadow.frag.glsl"));
    deferred.directionalLightWithoutShadow.link();

    // @todo define different SSR quality levels
    deferred.reflections.init();
    deferred.reflections.attachShader(Gm_CompileVertexShader("./gamma/opengl/shaders/quad.vert.glsl"));
    deferred.reflections.attachShader(Gm_CompileFragmentShader("./gamma/opengl/shaders/deferred/reflections.frag.glsl"));
    deferred.reflections.link();

    deferred.skybox.init();
    deferred.skybox.attachShader(Gm_CompileVertexShader("./gamma/opengl/shaders/quad.vert.glsl"));
    deferred.skybox.attachShader(Gm_CompileFragmentShader("./gamma/opengl/shaders/deferred/skybox.frag.glsl"));
    deferred.skybox.link();

    #if GAMMA_DEVELOPER_MODE
      deferred.gBufferLayers.init();
      deferred.gBufferLayers.attachShader(Gm_CompileVertexShader("./gamma/opengl/shaders/quad.vert.glsl"));
      deferred.gBufferLayers.attachShader(Gm_CompileFragmentShader("./gamma/opengl/shaders/deferred/g-buffer-preview.frag.glsl"));
      deferred.gBufferLayers.link();
    #endif

    deferred.lightDisc.init();

    // Initialize post shaders
    deferred.debanding.init();
    deferred.debanding.attachShader(Gm_CompileVertexShader("./gamma/opengl/shaders/quad.vert.glsl"));
    deferred.debanding.attachShader(Gm_CompileFragmentShader("./gamma/opengl/shaders/deband.frag.glsl"));
    deferred.debanding.link();

    // Initialize remaining shaders
    screen.init();
    screen.attachShader(Gm_CompileVertexShader("./gamma/opengl/shaders/quad.vert.glsl"));
    screen.attachShader(Gm_CompileFragmentShader("./gamma/opengl/shaders/screen.frag.glsl"));
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
    deferred.g_buffer.destroy();
    deferred.post_buffer.destroy();
    deferred.lightDisc.destroy();

    deferred.geometry.destroy();
    deferred.emissives.destroy();
    deferred.pointLightWithoutShadow.destroy();
    deferred.directionalLightWithoutShadow.destroy();
    deferred.copyFrame.destroy();
    deferred.reflections.destroy();
    deferred.skybox.destroy();
    deferred.debanding.destroy();

    #if GAMMA_DEVELOPER_MODE
      deferred.gBufferLayers.destroy();
    #endif

    glDeleteBuffers(1, &forward.lightsUbo);
    glDeleteTextures(1, &screenTexture);

    SDL_GL_DeleteContext(glContext);
  }

  void OpenGLRenderer::createMesh(Mesh* mesh) {
    glMeshes.push_back(new OpenGLMesh(mesh));

    #if GAMMA_DEVELOPER_MODE
      uint32 totalVertices = mesh->vertices.size();
      uint32 faces = mesh->faceElements.size() / 3;

      Console::log("[Gamma] Mesh created! (", totalVertices, " vertices, ", faces, " triangles)");
    #endif
  }

  void OpenGLRenderer::createShadowcaster(Light* mesh) {
    // @todo
    Console::log("[Gamma] Shadowcaster created!");
  }

  void OpenGLRenderer::destroyMesh(Mesh* mesh) {
    // @todo
    Console::log("[Gamma] Mesh destroyed!");
  }

  void OpenGLRenderer::destroyShadowcaster(Light* mesh) {
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
    AbstractScene& scene = *AbstractScene::active;
    uint32 sceneFlags = scene.getFlags();
    bool hasReflectiveObjects = false;

    if (sceneFlags & SceneFlags::MODE_VSYNC && SDL_GL_GetSwapInterval() == 0) {
      SDL_GL_SetSwapInterval(1);

      #if GAMMA_DEVELOPER_MODE
        Console::log("[Gamma] V-Sync enabled");
      #endif
    } else if (!(sceneFlags & SceneFlags::MODE_VSYNC) && SDL_GL_GetSwapInterval() == 1) {
      SDL_GL_SetSwapInterval(0);

      #if GAMMA_DEVELOPER_MODE
        Console::log("[Gamma] V-Sync disabled");
      #endif
    }

    // Set G-Buffer as render target
    deferred.g_buffer.write();

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
    Matrix4f projection = Matrix4f::projection({ internalWidth, internalHeight }, 45.0f, 1.0f, 10000.0f).transpose();

    Matrix4f view = (
      Matrix4f::rotation(camera.orientation.toVec3f()) *
      Matrix4f::translation(camera.position.invert().gl())
    ).transpose();

    deferred.geometry.use();
    deferred.geometry.setMatrix4f("projection", projection);
    deferred.geometry.setMatrix4f("view", view);
    deferred.geometry.setInt("meshTexture", 0);
    deferred.geometry.setInt("meshNormalMap", 1);

    GLenum primitiveMode = sceneFlags & SceneFlags::MODE_WIREFRAME
      ? GL_LINE_STRIP
      : GL_TRIANGLES;

    // @todo render emissive objects
    // glStencilMask(StencilType::EMISSIVE_OBJECTS);

    // Render reflective objects
    glStencilMask(StencilType::REFLECTIVE_OBJECTS);

    for (auto* glMesh : glMeshes) {
      if (glMesh->isReflective()) {
        if (glMesh->getObjectCount() > 0) {
          hasReflectiveObjects = true;
        }

        deferred.geometry.setBool("hasTexture", glMesh->hasTexture());
        deferred.geometry.setBool("hasNormalMap", glMesh->hasNormalMap());

        glMesh->render(primitiveMode);
      }
    }

    // Render non-emissive, non-reflective objects
    glStencilMask(StencilType::NON_EMISSIVE_OBJECTS);

    for (auto* glMesh : glMeshes) {
      if (!glMesh->isReflective()) {
        deferred.geometry.setBool("hasTexture", glMesh->hasTexture());
        deferred.geometry.setBool("hasNormalMap", glMesh->hasNormalMap());

        glMesh->render(primitiveMode);
      }
    }

    // Lighting pass; read from G-Buffer and preemptively write to post buffer
    deferred.g_buffer.read();
    deferred.post_buffer.write();

    glViewport(0, 0, internalWidth, internalHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);
    glStencilFunc(GL_LESS, StencilType::REFLECTIVE_OBJECTS - 1, 0xFF);
    glStencilMask(0x00);

    // Non-shadowcaster lighting pass
    auto& lights = scene.getLights();
    Matrix4f inverseProjection = projection.inverse();
    Matrix4f inverseView = view.inverse();

    // @todo don't reallocate on every frame
    std::vector<Light> pointLightsWithoutShadow;
    std::vector<Light> directionalLightsWithoutShadow;

    for (auto& light : lights) {
      // @todo ignore shadowcaster lights
      switch (light.type) {
        case LightType::POINT:
          pointLightsWithoutShadow.push_back(light);
          break;
        case LightType::DIRECTIONAL:
          directionalLightsWithoutShadow.push_back(light);
          break;
      }
    }

    // Render point lights (non-shadowcasters)
    if (pointLightsWithoutShadow.size() > 0) {
      auto& shader = deferred.pointLightWithoutShadow;

      shader.use();
      shader.setInt("colorAndDepth", 0);
      shader.setInt("normalAndSpecularity", 1);
      shader.setVec3f("cameraPosition", camera.position);
      shader.setMatrix4f("inverseProjection", inverseProjection);
      shader.setMatrix4f("inverseView", inverseView);

      deferred.lightDisc.draw(pointLightsWithoutShadow);
    }

    // Render directional lights (non-shadowcasters)
    if (directionalLightsWithoutShadow.size() > 0) {
      auto& shader = deferred.directionalLightWithoutShadow;

      shader.use();
      shader.setVec4f("transform", { 0.0f, 0.0f, 1.0f, 1.0f });
      shader.setInt("colorAndDepth", 0);
      shader.setInt("normalAndSpecularity", 1);
      shader.setVec3f("cameraPosition", camera.position);
      shader.setMatrix4f("inverseProjection", inverseProjection);
      shader.setMatrix4f("inverseView", inverseView);

      for (uint32 i = 0; i < directionalLightsWithoutShadow.size(); i++) {
        auto& light = directionalLightsWithoutShadow[i];
        std::string indexedLight = "lights[" + std::to_string(i) + "]";

        shader.setVec3f(indexedLight + ".color", light.color);
        shader.setFloat(indexedLight + ".power", light.power);
        shader.setVec3f(indexedLight + ".direction", light.direction);
      }

      OpenGLScreenQuad::render();
    }

    // @todo shadowed lighting pass

    glDisable(GL_BLEND);

    if (hasReflectiveObjects) {
      // Copy render pass up to this point back into G-Buffer, attachment 2
      deferred.post_buffer.read();
      deferred.g_buffer.write();

      deferred.copyFrame.use();
      deferred.copyFrame.setVec4f("transform", { 0.0f, 0.0f, 1.0f, 1.0f });
      deferred.copyFrame.setInt("colorAndDepth", 0);

      OpenGLScreenQuad::render();

      // Continue writing to post buffer
      deferred.g_buffer.read();
      // @todo investigate writing into a lower-res 'reflection buffer'
      // and writing that into the post buffer again
      deferred.post_buffer.write();

      // Render reflections (screen-space + skybox)
      // @bug unlit objects don't get reflected + if no lighting
      // sources are defined, nothing gets reflected
      glStencilFunc(GL_EQUAL, StencilType::REFLECTIVE_OBJECTS, 0xFF);

      deferred.reflections.use();
      deferred.reflections.setVec4f("transform", { 0.0f, 0.0f, 1.0f, 1.0f });
      deferred.reflections.setInt("normalAndSpecularity", 1);
      deferred.reflections.setInt("colorAndDepth", 2);
      deferred.reflections.setVec3f("cameraPosition", camera.position);
      deferred.reflections.setMatrix4f("view", view);
      deferred.reflections.setMatrix4f("inverseView", inverseView);
      deferred.reflections.setMatrix4f("projection", projection);
      deferred.reflections.setMatrix4f("inverseProjection", inverseProjection);

      OpenGLScreenQuad::render();
    }

    // Render skybox
    glStencilFunc(GL_EQUAL, 0x00, 0xFF);

    deferred.skybox.use();
    deferred.skybox.setVec4f("transform", { 0.0f, 0.0f, 1.0f, 1.0f });
    deferred.skybox.setVec3f("cameraPosition", camera.position);
    deferred.skybox.setMatrix4f("inverseProjection", inverseProjection);
    deferred.skybox.setMatrix4f("inverseView", inverseView);

    OpenGLScreenQuad::render();

    // @todo render translucent objects
    // glEnable(GL_CULL_FACE);
    // glEnable(GL_DEPTH_TEST);

    // Post-processing pass
    deferred.post_buffer.read();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glViewport(0, 0, Window::size.width, Window::size.height);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glStencilMask(0xFF);

    // @todo consider removing debanding if it's not as much of a problem
    // with fully textured/normal-mapped surfaces
    deferred.debanding.use();
    deferred.debanding.setVec4f("transform", { 0.0f, 0.0f, 1.0f, 1.0f });

    OpenGLScreenQuad::render();

    #if GAMMA_DEVELOPER_MODE
      deferred.g_buffer.read();

      deferred.gBufferLayers.use();
      deferred.gBufferLayers.setInt("colorAndDepth", 0);
      deferred.gBufferLayers.setInt("normalAndSpecularity", 1);
      deferred.gBufferLayers.setVec4f("transform", { 0.4f, 0.8f, 0.575f, 0.15f });

      OpenGLScreenQuad::render();
    #endif
  }

  void OpenGLRenderer::renderForward() {
    // @todo
  }

  void OpenGLRenderer::renderSurfaceToScreen(SDL_Surface* surface, uint32 x, uint32 y) {
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

    OpenGLScreenQuad::render();
  }

  void OpenGLRenderer::renderText(TTF_Font* font, const char* message, uint32 x, uint32 y) {
    SDL_Color color = { 255, 255, 255 };
    SDL_Surface* text = TTF_RenderText_Blended(font, message, color);

    renderSurfaceToScreen(text, x, y);

    SDL_FreeSurface(text);
  }
}