#pragma once

#include "math/plane.h"
#include "system/AbstractRenderer.h"
#include "system/traits.h"

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