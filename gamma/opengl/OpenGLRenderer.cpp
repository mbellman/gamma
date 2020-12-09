#include "SDL.h"
#include "glew.h"
#include "SDL_opengl.h"
#include "glut.h"
#include "opengl/OpenGLRenderer.h"
#include "system/AbstractController.h"
#include "system/object.h"

namespace Gamma {
  /**
   * OpenGLRenderer
   * --------------
   */
  void OpenGLRenderer::init() {
    flags = OpenGLRenderFlags::DEFERRED_PATH | OpenGLRenderFlags::SHADOWS;

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
    deferred.geometry.attachShader(Gm_CompileVertexShader("geometry.vertex.glsl"));
    deferred.geometry.attachShader(Gm_CompileFragmentShader("geometry.fragment.glsl"));
    deferred.geometry.link();
  }

  void OpenGLRenderer::render() {
    if (flags & OpenGLRenderFlags::DEFERRED_PATH) {
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
  }

  void OpenGLRenderer::renderForward() {
    // @TODO
  }

  void OpenGLRenderer::watch(AbstractController* controller) {
    controller->onMeshCreated([=](Mesh* mesh) {

    });

    controller->onMeshDestroyed([=](Mesh* mesh) {

    });
  }
}