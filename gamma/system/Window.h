#pragma once

#include "math/plane.h"
#include "system/AbstractRenderer.h"
#include "system/AbstractScene.h"
#include "system/Commander.h"
#include "system/macros.h"
#include "system/traits.h"
#include "system/type_aliases.h"

// @todo combine GmWindow/GmGameContext into GmContext
struct GmWindow {
  bool closed = false;
  Gamma::Commander commander;
  TTF_Font* font_sm = nullptr;
  TTF_Font* font_lg = nullptr;
  Gamma::AbstractRenderer* renderer = nullptr;
  SDL_Window* sdl_window = nullptr;
  Gamma::Area<Gamma::uint32> size;
};

struct GmGameContext {
  // @todo remove the need for an AbstractScene and add
  // appropriate fields to GmGameContext to manage scenes
  Gamma::AbstractScene* scene = nullptr;
};

enum GmRenderMode {
  OPENGL,
  VULKAN
};

GmWindow* Gm_CreateWindow();
GmGameContext* Gm_CreateGameContext(Gamma::AbstractScene* scene);  // @todo remove the need for scene
void Gm_SetRenderMode(GmWindow* window, GmRenderMode mode);
void Gm_HandleEvents(GmWindow* window, GmGameContext* context);
void Gm_RenderScene(GmWindow* window, GmGameContext* context);
void Gm_DestroyWindow(GmWindow* window);
void Gm_DestroyGameContext(GmGameContext* context);

namespace Gamma {
  enum RenderMode {
    OPENGL,
    VULKAN
  };

  class Window {
  public:
    static Area<uint32> size;

    Window();

    void open();
    void setController(AbstractController* controller);
    void setRenderMode(RenderMode mode);
    void setScreenRegion(const Region<uint32>& region);
    void setTitle(const char* title);

  private:
    Commander commander;
    SDL_Window* sdl_window = nullptr;
    TTF_Font* font_OpenSans_sm = nullptr;
    TTF_Font* font_OpenSans_lg = nullptr;
    AbstractController* controller = nullptr;
    AbstractRenderer* renderer = nullptr;

    void bindEvents();
    void destroyRenderer();
  };
}