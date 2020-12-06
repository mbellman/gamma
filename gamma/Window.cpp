#include "SDL.h"
#include "Window.h"

using namespace Gamma;

Window::~Window() {
  delete renderer;
}

void Window::open() {
  if (renderer == nullptr) {
    return;
  }

  auto* window = SDL_CreateWindow("Window!", region.x, region.y, region.width, region.height, SDL_WINDOW_RESIZABLE);

  SDL_Event event;
  bool didCloseWindow = false;

  while (!didCloseWindow) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        didCloseWindow = true;
        break;
      }
    }

    SDL_Delay(1);
  }

  SDL_DestroyWindow(window);
}

void Window::setRegion(const Region<unsigned int>& region) {
  this->region = region;
}

void Window::setRenderer(AbstractRenderer* renderer) {
  this->renderer = renderer;
}