#pragma once

#include "system/traits.h"

struct SDL_Window;

namespace Gamma {
  struct Mesh;

  class AbstractRenderer : public Initable, public Renderable, public Destroyable {
  public:
    AbstractRenderer(SDL_Window* sdl_window): sdl_window(sdl_window) {};
    virtual ~AbstractRenderer() {};

    virtual void createMesh(Mesh* mesh) {};
    virtual void destroyMesh(Mesh* mesh) {};

  protected:
    SDL_Window* sdl_window = nullptr;
  };
}