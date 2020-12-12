#pragma once

#include "system/traits.h"

struct SDL_Window;

namespace Gamma {
  struct Mesh;
  struct Light;

  class AbstractRenderer : public Initable, public Renderable, public Destroyable {
  public:
    AbstractRenderer(SDL_Window* sdl_window): sdl_window(sdl_window) {};
    virtual ~AbstractRenderer() {};

    virtual void createMesh(Mesh* mesh) {};
    virtual void createShadowcaster(Light* light) {};
    virtual void destroyMesh(Mesh* mesh) {};
    virtual void destroyShadowcaster(Light* light) {};

  protected:
    SDL_Window* sdl_window = nullptr;
  };
}