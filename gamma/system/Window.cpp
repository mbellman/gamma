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

GmContext* Gm_CreateContext() {
  auto* context = new GmContext();

  // @todo create Gm_Init() and move there
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

  return context;
}

void Gm_SetRenderMode(GmContext* context, GmRenderMode mode) {
  if (context->renderer) {
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
  context->scene = scene;

  context->scene->init();

  // @todo add renderable meshes
}

void Gm_AddRenderableMesh(GmContext* context, const Gamma::Mesh* mesh) {
  // @todo
}

void Gm_HandleEvents(GmContext* context) {
  auto* activeScene = context->scene;
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
        }

        break;
      default:
        break;
    }

    if (activeScene != nullptr && !context->commander.isOpen()) {
      activeScene->input.handleEvent(event);
    }

    #if GAMMA_DEVELOPER_MODE
      context->commander.input.handleEvent(event);
    #endif
  }
}

void Gm_RenderScene(GmContext* context) {
  context->renderer->render();
  context->renderer->present();
}

void Gm_DestroyContext(GmContext* context) {
  if (context->scene) {
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
   */
  Area<uint32> Window::size = { 0, 0 };

  Window::Window() {
    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();
    IMG_Init(IMG_INIT_PNG);

    sdl_window = SDL_CreateWindow(
      "Gamma",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      640, 480,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    font_OpenSans_sm = TTF_OpenFont("./fonts/OpenSans-Regular.ttf", 16);
    font_OpenSans_lg = TTF_OpenFont("./fonts/OpenSans-Regular.ttf", 22);

    Window::size = { 640, 480 };
  }

  void Window::bindEvents() {
    if (AbstractScene::active != nullptr) {
      // @todo subscribe to active scene mesh/light events
    }

    controller->on<AbstractScene*>("scene-created", [=](AbstractScene* scene) {
      // @todo clear all existing renderer meshes/lights when scenes change
      scene->on<const Mesh*>("mesh-created", [=](auto* mesh) {
        renderer->createMesh(mesh);
      });

      scene->on<const Mesh*>("mesh-destroyed", [=](auto* mesh) {
        renderer->destroyMesh(mesh);
      });

      scene->on<const Light*>("shadowcaster-created", [=](auto* light) {
        renderer->createShadowMap(light);
      });

      scene->on<const Light*>("shadowcaster-destroyed", [=](auto* light) {
        renderer->destroyShadowMap(light);
      });
    });
  }

  void Window::destroyRenderer() {
    if (renderer != nullptr) {
      renderer->destroy();

      delete renderer;

      renderer = nullptr;
    }
  }

  void Window::open() {
    SDL_Event event;
    bool didCloseWindow = false;
    uint32 lastTick = SDL_GetTicks();

    Averager<5, uint32> fpsAverager;
    Averager<5, uint64> frameTimeAverager;

    // Main window loop
    while (!didCloseWindow) {
      AbstractScene* activeScene = AbstractScene::active;

      while (SDL_PollEvent(&event)) {
        switch (event.type) {
          case SDL_QUIT:
            didCloseWindow = true;
            break;
          case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
              Window::size = {
                (uint32)event.window.data1,
                (uint32)event.window.data2
              };
            }

            break;
          default:
            break;
        }

        if (activeScene != nullptr && !commander.isOpen()) {
          activeScene->input.handleEvent(event);
        }

        #if GAMMA_DEVELOPER_MODE
          commander.input.handleEvent(event);
        #endif
      }

      if (activeScene != nullptr) {
        float dt = (float)(SDL_GetTicks() - lastTick) / 1000.0f;
        lastTick = SDL_GetTicks();

        Camera::active = &activeScene->camera;

        activeScene->updateScene(dt);

        if (renderer != nullptr) {
          // @todo create profiler helpers for this
          auto getTime = Gm_CreateTimer();

          renderer->render();

          #if GAMMA_DEVELOPER_MODE
            // Display stats
            // @todo make this a method
            auto& resolution = renderer->getInternalResolution();
            auto& renderStats = renderer->getRenderStats();
            auto& sceneStats = activeScene->getStats();
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
            renderer->renderText(font_OpenSans_sm, fpsLabel.c_str(), 25, 25);
            renderer->renderText(font_OpenSans_sm, frameTimeLabel.c_str(), 25, 50);
            renderer->renderText(font_OpenSans_sm, resolutionLabel.c_str(), 25, 75);
            renderer->renderText(font_OpenSans_sm, vertsLabel.c_str(), 25, 100);
            renderer->renderText(font_OpenSans_sm, trisLabel.c_str(), 25, 125);
            renderer->renderText(font_OpenSans_sm, memoryLabel.c_str(), 25, 150);

            // Display command line
            if (commander.isOpen()) {
              std::string caret = SDL_GetTicks() % 1000 < 500 ? "_" : "  ";
              Vec3f fg = Vec3f(0.0f, 1.0f, 0.0f);
              Vec4f bg = Vec4f(0.0f, 0.0f, 0.0f, 0.8f);

              renderer->renderText(font_OpenSans_lg, ("> " + commander.getCommand() + caret).c_str(), 25, Window::size.height - 200, fg, bg);
            }

            // Display console messages
            auto* message = Console::getFirstMessage();
            uint8 messageIndex = 0;

            // @todo clear messages after a set duration
            while (message != nullptr) {
              renderer->renderText(font_OpenSans_sm, message->text.c_str(), 25, Window::size.height - 150 + (messageIndex++) * 25);

              message = message->next;
            }
          #endif

          renderer->present();

          auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(getTime()).count();

          uint32 fps = (uint32)(1000000 / (float)microseconds);

          fpsAverager.add(fps);
          frameTimeAverager.add(microseconds);
        }
      }

      Gm_SavePreviousFlags();
    }

    // Post-quit cleanup
    destroyRenderer();

    controller->destroy();

    delete controller;

    IMG_Quit();

    TTF_CloseFont(font_OpenSans_sm);
    TTF_CloseFont(font_OpenSans_lg);
    TTF_Quit();

    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
  }

  void Window::setController(AbstractController* controller) {
    this->controller = controller;

    if (renderer != nullptr) {
      bindEvents();
    }

    controller->init();
  }

  void Window::setRenderMode(RenderMode mode) {
    destroyRenderer();

    switch (mode) {
      case RenderMode::OPENGL:
        renderer = new OpenGLRenderer(sdl_window);
        break;
      case RenderMode::VULKAN:
        // @todo
        break;
    }

    if (renderer != nullptr) {
      renderer->init();

      if (AbstractScene::active != nullptr) {
        // @todo get all active meshes/lights and create them in the renderer instance
      }

      if (controller != nullptr) {
        bindEvents();
      }
    }
  }

  void Window::setScreenRegion(const Region<uint32>& region) {
    SDL_SetWindowPosition(sdl_window, region.x, region.y);
    SDL_SetWindowSize(sdl_window, region.width, region.height);
  }

  void Window::setTitle(const char* title) {
    SDL_SetWindowTitle(sdl_window, title);
  }
}