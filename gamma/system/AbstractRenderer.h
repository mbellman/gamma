#pragma once

#include "system/traits.h"
#include "system/type_aliases.h"

#include "SDL.h"
#include "SDL_ttf.h"

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
    virtual void present() {};
    virtual void renderText(TTF_Font* font, const char* message, uint32 x, uint32 y) {};

  protected:
    SDL_Window* sdl_window = nullptr;
  };
}