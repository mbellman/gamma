#include "SDL.h"
#include "Window.h"
#include "opengl/OpenGLRenderer.h"

using namespace Gamma;

Window::Window() {
  SDL_Init(SDL_INIT_EVERYTHING);

  sdl_window = SDL_CreateWindow("Gamma", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
}

void Window::clearRenderer() {
  if (renderer != nullptr) {
    renderer->destroy();

    delete renderer;

    renderer = nullptr;
  }
}

void Window::open() {
  SDL_Event event;
  bool didCloseWindow = false;

  // Main window loop
  while (!didCloseWindow) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          didCloseWindow = true;
          break;
        }
    }

    if (renderer != nullptr) {
      renderer->render();
    }

    SDL_Delay(1);
  }

  // Post-quit cleanup
  clearRenderer();

  SDL_DestroyWindow(sdl_window);
}

void Window::setRenderMode(RenderMode mode) {
  clearRenderer();

  switch (mode) {
    case RenderMode::OPENGL:
      renderer = new OpenGLRenderer(sdl_window);
      break;
    case RenderMode::VULKAN:
      // Not implemented
      break;
  }

  if (renderer != nullptr) {
    renderer->init();
  }
}

void Window::setScreenRegion(const Region<unsigned int>& region) {
  SDL_SetWindowPosition(sdl_window, region.x, region.y);
  SDL_SetWindowSize(sdl_window, region.width, region.height);
}