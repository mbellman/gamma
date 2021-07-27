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

  /**
   * OpenGLRenderer
   * --------------
   */
  void OpenGLRenderer::init() {
    flags = OpenGLRenderFlags::RENDER_DEFERRED | OpenGLRenderFlags::RENDER_SHADOWS;

    // Initialize OpenGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);

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
    // @TODO

    // Initialize dynamic lights UBO
    // @TODO buffer lights to forward renderer geometry shader
    glGenBuffers(1, &forward.lightsUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, forward.lightsUbo);
    glBufferData(GL_UNIFORM_BUFFER, MAX_LIGHTS * sizeof(Light), 0, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, forward.lightsUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Initialize deferred renderer
    // @TODO define separate OpenGLDeferredRenderer/OpenGLForwardRenderer classes
    deferred.g_buffer.init();
    deferred.g_buffer.setSize(internalResolution);
    deferred.g_buffer.addColorAttachment(ColorFormat::RGBA);  // (RGB) Color, (A) Depth
    deferred.g_buffer.addColorAttachment(ColorFormat::RGBA);  // (RGB) Normal, (A) Specularity
    deferred.g_buffer.addDepthStencilAttachment();
    deferred.g_buffer.bindColorAttachments();

    deferred.geometry.init();
    deferred.geometry.attachShader(Gm_CompileVertexShader("shaders/deferred/geometry.vert.glsl"));
    deferred.geometry.attachShader(Gm_CompileFragmentShader("shaders/deferred/geometry.frag.glsl"));
    deferred.geometry.link();

    deferred.illumination.init();
    deferred.illumination.attachShader(Gm_CompileVertexShader("shaders/deferred/lights.vert.glsl"));
    deferred.illumination.attachShader(Gm_CompileFragmentShader("shaders/deferred/lights.frag.glsl"));
    deferred.illumination.link();

    #if GAMMA_SHOW_G_BUFFER_LAYERS
      deferred.gBufferLayers.init();
      deferred.gBufferLayers.attachShader(Gm_CompileVertexShader("shaders/quad.vert.glsl"));
      deferred.gBufferLayers.attachShader(Gm_CompileFragmentShader("shaders/deferred/g-buffer-preview.frag.glsl"));
      deferred.gBufferLayers.link();
    #endif

    deferred.lightDisc.init();

    // Initialize post effects
    post.debanding.buffer.init();
    post.debanding.buffer.setSize(internalResolution);
    post.debanding.buffer.addColorAttachment(ColorFormat::RGBA);  // (RGB) Color, (A) Depth
    deferred.g_buffer.shareDepthStencilAttachment(post.debanding.buffer);
    post.debanding.buffer.bindColorAttachments();

    post.debanding.shader.init();
    post.debanding.shader.attachShader(Gm_CompileVertexShader("shaders/quad.vert.glsl"));
    post.debanding.shader.attachShader(Gm_CompileFragmentShader("shaders/deband.frag.glsl"));
    post.debanding.shader.link();

    // Initialize remaining shaders
    screen.init();
    screen.attachShader(Gm_CompileVertexShader("shaders/quad.vert.glsl"));
    screen.attachShader(Gm_CompileFragmentShader("shaders/screen.frag.glsl"));
    screen.link();
  }

  void OpenGLRenderer::render() {
    // @TODO define separate OpenGLDeferredRenderer/OpenGLForwardRenderer classes
    if (flags & OpenGLRenderFlags::RENDER_DEFERRED) {
      renderDeferred();
    } else {
      renderForward();
    }
  }

  void OpenGLRenderer::destroy() {
    deferred.g_buffer.destroy();
    deferred.geometry.destroy();
    deferred.illumination.destroy();
    deferred.emissives.destroy();
    deferred.lightDisc.destroy();

    #if GAMMA_SHOW_G_BUFFER_LAYERS
      deferred.gBufferLayers.destroy();
    #endif

    glDeleteBuffers(1, &forward.lightsUbo);
    glDeleteTextures(1, &screenTexture);

    SDL_GL_DeleteContext(glContext);
  }

  void OpenGLRenderer::createMesh(Mesh* mesh) {
    glMeshes.push_back(new OpenGLMesh(mesh));
  }

  void OpenGLRenderer::createShadowcaster(Light* mesh) {
    // @TODO
    log("Shadowcaster created!");
  }

  void OpenGLRenderer::destroyMesh(Mesh* mesh) {
    // @TODO
    log("Mesh destroyed!");
  }

  void OpenGLRenderer::destroyShadowcaster(Light* mesh) {
    // @TODO
    log("Shadowcaster destroyed!");
  }

  void OpenGLRenderer::present() {
    SDL_GL_SwapWindow(sdl_window);
  }

  void OpenGLRenderer::renderDeferred() {
    // Set G-Buffer as render target
    deferred.g_buffer.write();

    uint32 internalWidth = internalResolution.width;
    uint32 internalHeight = internalResolution.height;

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

    GLenum primitiveMode = AbstractScene::active->flags & SceneFlags::MODE_WIREFRAME
      ? GL_LINE_STRIP
      : GL_TRIANGLES;

    // Render non-emissive objects
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    for (auto* glMesh : glMeshes) {
      glMesh->render(primitiveMode);
    }

    glStencilMask(0x00);

    // @TODO render emissive objects

    // Lighting pass; read from G-Buffer and preemptively write to post-processing pipeline
    deferred.g_buffer.read();
    post.debanding.buffer.write();

    glViewport(0, 0, internalWidth, internalHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);
    glStencilFunc(GL_EQUAL, 1, 0xFF);

    // Non-shadowed lighting pass
    auto& lights = AbstractScene::active->getLights();

    deferred.illumination.use();
    deferred.illumination.setVec4f("transform", { 0.0f, 0.0f, 1.0f, 1.0f });
    deferred.illumination.setInt("colorAndDepth", 0);
    deferred.illumination.setInt("normalAndSpecularity", 1);
    deferred.illumination.setVec3f("cameraPosition", camera.position);
    deferred.illumination.setMatrix4f("inverseProjection", projection.inverse());
    deferred.illumination.setMatrix4f("inverseView", view.inverse());

    deferred.lightDisc.draw(lights);

    // @TODO shadowed lighting pass

    // Post-processing pass
    post.debanding.buffer.read();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glViewport(0, 0, Window::size.width, Window::size.height);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glStencilMask(0xFF);

    post.debanding.shader.use();
    post.debanding.shader.setVec4f("transform", { 0.0f, 0.0f, 1.0f, 1.0f });

    OpenGLScreenQuad::render();

    #if GAMMA_SHOW_G_BUFFER_LAYERS
      deferred.g_buffer.read();

      deferred.gBufferLayers.use();
      deferred.gBufferLayers.setInt("colorAndDepth", 0);
      deferred.gBufferLayers.setInt("normalAndSpecularity", 1);
      deferred.gBufferLayers.setVec4f("transform", { 0.4f, 0.8f, 0.575f, 0.15f });

      OpenGLScreenQuad::render();
    #endif
  }

  void OpenGLRenderer::renderForward() {
    // @TODO
  }

  void OpenGLRenderer::renderSurfaceToScreen(SDL_Surface* surface, uint32 x, uint32 y) {
    float offsetX = -1.0f + (x + surface->w) / (float)Window::size.width;
    float offsetY = 1.0f - (y + surface->h) / (float)Window::size.height;
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