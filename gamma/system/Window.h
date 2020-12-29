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
    static Area<uint32> size;

    Window();

    void open();
    void setController(AbstractController* controller);
    void setRenderMode(RenderMode mode);
    void setScreenRegion(const Region<uint32>& region);

  private:
    SDL_Window* sdl_window = nullptr;
    AbstractController* controller = nullptr;
    AbstractRenderer* renderer = nullptr;

    void bindControllerEvents();
    void destroyRenderer();
  };
}