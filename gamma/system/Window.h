#pragma once

#include "math/plane.h"
#include "performance/tools.h"
#include "system/AbstractRenderer.h"
#include "system/AbstractScene.h"
#include "system/Commander.h"
#include "system/entities.h"
#include "system/macros.h"
#include "system/traits.h"
#include "system/type_aliases.h"

enum GmRenderMode {
  OPENGL,
  VULKAN
};

struct GmContext {
  // @todo replace AbstractScene with a scene management struct
  Gamma::AbstractScene* scene = nullptr;
  Gamma::AbstractRenderer* renderer = nullptr;
  Gamma::uint32 lastTick = 0;
  Gamma::uint64 frameStartMicroseconds = 0;
  Gamma::Averager<5, Gamma::uint32> fpsAverager;
  Gamma::Averager<5, Gamma::uint64> frameTimeAverager;
  Gamma::Commander commander;

  struct {
    bool closed = false;
    TTF_Font* font_sm = nullptr;
    TTF_Font* font_lg = nullptr;
    SDL_Window* sdl_window = nullptr;
    Gamma::Area<Gamma::uint32> size;
  } window;
};

GmContext* Gm_CreateContext();
void Gm_OpenWindow(GmContext* context, const Gamma::Area<Gamma::uint32>& size);
void Gm_SetRenderMode(GmContext* context, GmRenderMode mode);
void Gm_SetScene(GmContext* context, Gamma::AbstractScene* scene);
float Gm_GetDeltaTime(GmContext* context);
void Gm_LogFrameStart(GmContext* context);
void Gm_HandleEvents(GmContext* context);
void Gm_RenderScene(GmContext* context);
void Gm_LogFrameEnd(GmContext* context);
void Gm_DestroyContext(GmContext* context);

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