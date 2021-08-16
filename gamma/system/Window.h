#pragma once

#include <string>


#include "math/plane.h"
#include "system/Commander.h"
#include "system/traits.h"
#include "system/type_aliases.h"

#include "SDL_ttf.h"

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
    Commander commander;
    SDL_Window* sdl_window = nullptr;
    TTF_Font* font_OpenSans = nullptr;
    AbstractController* controller = nullptr;
    AbstractRenderer* renderer = nullptr;

    void bindEvents();
    void destroyRenderer();
  };
}