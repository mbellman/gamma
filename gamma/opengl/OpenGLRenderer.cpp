#include <cstdio>
#include <map>

#include "SDL.h"
#include "glew.h"
#include "SDL_opengl.h"
#include "glut.h"
#include "opengl/errors.h"
#include "opengl/OpenGLRenderer.h"
#include "opengl/OpenGLScreenQuad.h"
#include "system/AbstractController.h"
#include "system/AbstractScene.h"
#include "system/camera.h"
#include "system/console.h"
#include "system/entities.h"
#include "system/Window.h"

namespace Gamma {
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

    // Initialize framebuffers and shaders
    deferred.g_buffer.init();
    deferred.g_buffer.setSize({ 1920, 1080 });
    deferred.g_buffer.addColorAttachment(ColorFormat::RGBA);  // (RGB) Color, (A) Depth
    deferred.g_buffer.addColorAttachment(ColorFormat::RGBA);  // (RGB) Normal, (A) Specularity
    deferred.g_buffer.addDepthStencilAttachment();
    deferred.g_buffer.bindColorAttachments();

    deferred.geometry.init();
    deferred.geometry.attachShader(Gm_CompileVertexShader("shaders/deferred/geometry.vert.glsl"));
    deferred.geometry.attachShader(Gm_CompileFragmentShader("shaders/deferred/geometry.frag.glsl"));
    deferred.geometry.link();

    deferred.illumination.init();
    deferred.illumination.attachShader(Gm_CompileVertexShader("shaders/deferred/quad.vert.glsl"));
    deferred.illumination.attachShader(Gm_CompileFragmentShader("shaders/deferred/illumination.frag.glsl"));
    deferred.illumination.link();
  }

  void OpenGLRenderer::render() {
    if (flags & OpenGLRenderFlags::RENDER_DEFERRED) {
      renderDeferred();
    } else {
      renderForward();
    }

    SDL_GL_SwapWindow(sdl_window);
  }

  void OpenGLRenderer::destroy() {
    deferred.g_buffer.destroy();
    deferred.geometry.destroy();
    deferred.illumination.destroy();
    deferred.emissives.destroy();

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

  void OpenGLRenderer::renderDeferred() {
    // Clear G-Buffer
    deferred.g_buffer.write();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // Reset rendering state
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glCullFace(GL_BACK);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);
    glViewport(0, 0, 1920, 1080);

    // Render camera view to G-Buffer
    auto& camera = *Camera::active;
    Matrix4f projection = Matrix4f::projection({ 1920, 1080 }, 45.0f, 1.0f, 10000.0f).transpose();

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

    for (auto* glMesh : glMeshes) {
      glMesh->render(primitiveMode);
    }

    // Render illuminated camera view from G-Buffer layers
    // @TODO lighting, shadowing, and post-processing
    deferred.g_buffer.read();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glViewport(0, 0, Window::size.width, Window::size.height);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    deferred.illumination.use();
    deferred.illumination.setInt("colorAndDepth", 0);
    deferred.illumination.setInt("normalAndSpecularity", 1);
    deferred.illumination.setVec3f("cameraPosition", camera.position);
    deferred.illumination.setMatrix4f("inverseProjection", projection.inverse());
    deferred.illumination.setMatrix4f("inverseView", view.inverse());

    OpenGLScreenQuad::render();
  }

  void OpenGLRenderer::renderForward() {
    // @TODO
  }
}