#pragma once

#include "AbstractRenderer.h"

typedef void* SDL_GLContext;

namespace Gamma {
  class OpenGLRenderer final : public AbstractRenderer {
  public:
    OpenGLRenderer(SDL_Window* sdl_window): AbstractRenderer(sdl_window) {};
    ~OpenGLRenderer() {};

    virtual void onInit() override;
    virtual void onRender() override;
    virtual void onDestroy() override;

  private:
    SDL_GLContext glContext;
  };
}