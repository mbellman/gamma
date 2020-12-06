#include "opengl/OpenGLRenderer.h"
#include "SDL.h"
#include "glew.h"
#include "SDL_opengl.h"
#include "glut.h"

using namespace Gamma;

void OpenGLRenderer::onInit() {
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
}

void OpenGLRenderer::onRender() {
  // Reset rendering state
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_STENCIL_TEST);
  glCullFace(GL_BACK);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);

  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  SDL_GL_SwapWindow(sdl_window);
}

void OpenGLRenderer::onDestroy() {
  SDL_GL_DeleteContext(glContext);
}