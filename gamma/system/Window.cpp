#include <string>

#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_image.h"

#include "opengl/OpenGLRenderer.h"
#include "performance/benchmark.h"
#include "performance/tools.h"
#include "system/AbstractController.h"
#include "system/AbstractScene.h"
#include "system/console.h"
#include "system/flags.h"
#include "system/Window.h"

static void Gm_DisplayDevtools(GmContext* context) {
  using namespace Gamma;

  auto& renderer = *context->renderer;
  auto& resolution = renderer.getInternalResolution();
  auto& renderStats = renderer.getRenderStats();
  auto& sceneStats = context->scene->getStats();
  auto& fpsAverager = context->fpsAverager;
  auto& frameTimeAverager = context->frameTimeAverager;
  auto& commander = context->commander;
  auto* font_sm = context->window.font_sm;
  auto* font_lg = context->window.font_lg;
  uint64 averageFrameTime = frameTimeAverager.average();
  uint32 frameTimeBudget = uint32(100.0f * (float)averageFrameTime / 16667.0f);

  std::string fpsLabel = "FPS: "
    + std::to_string(fpsAverager.average())
    + ", low "
    + std::to_string(fpsAverager.low())
    + " (V-Sync " + (renderStats.isVSynced ? "ON" : "OFF") + ")";

  std::string frameTimeLabel = "Frame time: "
    + std::to_string(averageFrameTime)
    + "us, high "
    + std::to_string(frameTimeAverager.high())
    + " ("
    + std::to_string(frameTimeBudget)
    + "%)";

  std::string resolutionLabel = "Resolution: " + std::to_string(resolution.width) + " x " + std::to_string(resolution.height);
  std::string vertsLabel = "Verts: " + std::to_string(sceneStats.verts);
  std::string trisLabel = "Tris: " + std::to_string(sceneStats.tris);
  std::string memoryLabel = "GPU Memory: " + std::to_string(renderStats.gpuMemoryUsed) + "MB / " + std::to_string(renderStats.gpuMemoryTotal) + "MB";

  // @todo write a little helper for rendering labels less repetitively
  renderer.renderText(font_sm, fpsLabel.c_str(), 25, 25);
  renderer.renderText(font_sm, frameTimeLabel.c_str(), 25, 50);
  renderer.renderText(font_sm, resolutionLabel.c_str(), 25, 75);
  renderer.renderText(font_sm, vertsLabel.c_str(), 25, 100);
  renderer.renderText(font_sm, trisLabel.c_str(), 25, 125);
  renderer.renderText(font_sm, memoryLabel.c_str(), 25, 150);

  // Display command line
  if (commander.isOpen()) {
    std::string caret = SDL_GetTicks() % 1000 < 500 ? "_" : "  ";
    Vec3f fg = Vec3f(0.0f, 1.0f, 0.0f);
    Vec4f bg = Vec4f(0.0f, 0.0f, 0.0f, 0.8f);

    renderer.renderText(font_lg, ("> " + commander.getCommand() + caret).c_str(), 25, Window::size.height - 200, fg, bg);
  }

  // Display console messages
  auto* message = Console::getFirstMessage();
  uint8 messageIndex = 0;

  // @todo clear messages after a set duration
  while (message != nullptr) {
    renderer.renderText(font_sm, message->text.c_str(), 25, Window::size.height - 150 + (messageIndex++) * 25);

    message = message->next;
  }
}

GmContext* Gm_CreateContext() {
  auto* context = new GmContext();

  SDL_Init(SDL_INIT_EVERYTHING);
  TTF_Init();
  IMG_Init(IMG_INIT_PNG);

  // @todo Gm_OpenWindow(const char* title, const Area<uint32>& size)
  context->window.sdl_window = SDL_CreateWindow(
    "Gamma",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    640, 480,
    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
  );

  context->window.font_sm = TTF_OpenFont("./fonts/OpenSans-Regular.ttf", 16);
  context->window.font_lg = TTF_OpenFont("./fonts/OpenSans-Regular.ttf", 22);
  context->window.size = { 640, 480 };

  // @hack accommodate Window::size checks
  // @todo pass a GmContext* to AbstractRenderer, rather than SDL_Window*
  Gamma::Window::size = context->window.size;

  return context;
}

void Gm_SetRenderMode(GmContext* context, GmRenderMode mode) {
  if (context->renderer != nullptr) {
    context->renderer->destroy();

    delete context->renderer;

    context->renderer = nullptr;
  }

  switch (mode) {
    case GmRenderMode::OPENGL:
      context->renderer = new Gamma::OpenGLRenderer(context->window.sdl_window);
      break;
    case GmRenderMode::VULKAN:
      // @todo
      break;
  }

  if (context->renderer != nullptr) {
    context->renderer->init();
  }
}

// @todo remove this method once AbstractScene is replaced with a scene struct
void Gm_SetScene(GmContext* context, Gamma::AbstractScene* scene) {
  using namespace Gamma;

  context->scene = scene;

  AbstractScene::active = scene;

  // @todo remove Signaler/event-driven behavior here
  // once AbstractScene becomes a regular struct
  scene->on<const Mesh*>("mesh-created", [=](auto* mesh) {
    context->renderer->createMesh(mesh);
  });

  scene->on<const Mesh*>("mesh-destroyed", [=](auto* mesh) {
    context->renderer->destroyMesh(mesh);
  });

  scene->on<const Light*>("shadowcaster-created", [=](auto* light) {
    context->renderer->createShadowMap(light);
  });

  scene->on<const Light*>("shadowcaster-destroyed", [=](auto* light) {
    context->renderer->destroyShadowMap(light);
  });

  scene->init();
}

float Gm_GetDeltaTime(GmContext* context) {
  Gamma::uint32 ticks = SDL_GetTicks();
  float dt = float(ticks - context->lastTick) / 1000.0f;

  context->lastTick = ticks;

  return dt;
}

void Gm_LogFrameStart(GmContext* context) {
  context->frameStartMicroseconds = Gm_GetMicroseconds();
}

void Gm_HandleEvents(GmContext* context) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        context->window.closed = true;
        break;
      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          context->window.size = {
            (Gamma::uint32)event.window.data1,
            (Gamma::uint32)event.window.data2
          };

          // @hack accommodate Window::size checks
          // @todo pass a GmContext* to AbstractRenderer, rather than SDL_Window*
          Gamma::Window::size = context->window.size;
        }

        break;
      default:
        break;
    }

    if (context->scene != nullptr && !context->commander.isOpen()) {
      context->scene->input.handleEvent(event);
    }

    #if GAMMA_DEVELOPER_MODE
      context->commander.input.handleEvent(event);
    #endif
  }
}

void Gm_RenderScene(GmContext* context) {
  context->renderer->render();

  #if GAMMA_DEVELOPER_MODE
    Gm_DisplayDevtools(context);
  #endif

  context->renderer->present();

  // @todo move this elsewhere
  Gamma::Gm_SavePreviousFlags();
}

void Gm_LogFrameEnd(GmContext* context) {
  using namespace Gamma;

  uint64 frameTimeInMicroseconds = Gm_GetMicroseconds() - context->frameStartMicroseconds;
  uint32 fps = (uint32)(1000000.0f / (float)frameTimeInMicroseconds);

  context->fpsAverager.add(fps);
  context->frameTimeAverager.add(frameTimeInMicroseconds);
}

void Gm_DestroyContext(GmContext* context) {
  if (context->scene != nullptr) {
    context->scene->destroy();

    delete context->scene;
  }

  IMG_Quit();

  TTF_CloseFont(context->window.font_sm);
  TTF_CloseFont(context->window.font_lg);
  TTF_Quit();

  SDL_DestroyWindow(context->window.sdl_window);
  SDL_Quit();
}

namespace Gamma {
  /**
   * Window
   * ------
   *
   * @todo remove the need for this
   */
  Area<uint32> Window::size = { 0, 0 };
}