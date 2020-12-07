#pragma once

#include "system/traits.h"

struct SDL_Window;

namespace Gamma {
  class AbstractRenderer : public Initable, public Renderable, public Destroyable {
  public:
    AbstractRenderer(SDL_Window* sdl_window): sdl_window(sdl_window) {};
    virtual ~AbstractRenderer() {};

  protected:
    SDL_Window* sdl_window = nullptr;
  };
}