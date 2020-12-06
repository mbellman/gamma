#pragma once

#include "AbstractRenderer.h"
#include "Geometry.h"
#include "Traits.h"

struct SDL_Window;

namespace Gamma {
  enum RenderMode {
    OPENGL,
    VULKAN
  };

  class Window {
  public:
    Window();

    void open();
    void setRenderMode(RenderMode mode);
    void setScreenRegion(const Region<unsigned int>& region);

  private:
    SDL_Window* sdl_window = nullptr;
    AbstractRenderer* renderer = nullptr;

    void clearRenderer();
  };
}