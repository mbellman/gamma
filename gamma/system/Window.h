#pragma once

#include "math/plane.h"
#include "system/traits.h"
#include "system/type_aliases.h"

namespace Gamma {
  enum RenderMode {
    OPENGL,
    VULKAN
  };

  class AbstractController;
  class AbstractRenderer;

  class Window {
  public:
    Window();

    void open();
    void setController(AbstractController* controller);
    void setRenderMode(RenderMode mode);
    void setScreenRegion(const Region<unsigned int>& region);

  private:
    SDL_Window* sdl_window = nullptr;
    AbstractController* controller = nullptr;
    AbstractRenderer* renderer = nullptr;

    void clearRenderer();
  };
}