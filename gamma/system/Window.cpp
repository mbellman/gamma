#include "SDL.h"
#include "opengl/OpenGLRenderer.h"
#include "performance/benchmark.h"
#include "system/AbstractController.h"
#include "system/AbstractScene.h"
#include "system/console.h"
#include "system/Window.h"

namespace Gamma {
  /**
   * Window
   * ------
   */
  Area<uint32> Window::size = { 0, 0 };

  Window::Window() {
    SDL_Init(SDL_INIT_EVERYTHING);

    sdl_window = SDL_CreateWindow(
      "Gamma",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      640, 480,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    Window::size = { 640, 480 };
  }

  void Window::bindEvents() {
    if (AbstractScene::active != nullptr) {
      // @TODO: subscribe to active scene mesh/light events
    }

    controller->on<AbstractScene*>("scene-created", [=](AbstractScene* scene) {
      scene->on<Mesh*>("mesh-created", [=](auto* mesh) {
        renderer->createMesh(mesh);
      });

      scene->on<Mesh*>("mesh-destroyed", [=](auto* mesh) {
        renderer->destroyMesh(mesh);
      });

      scene->on<Light*>("shadowcaster-created", [=](auto* light) {
        renderer->createShadowcaster(light);
      });

      scene->on<Light*>("shadowcaster-destroyed", [=](auto* light) {
        renderer->destroyShadowcaster(light);
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

        if (activeScene != nullptr) {
          activeScene->handleEvent(event);
        }
      }

      if (activeScene != nullptr) {
        float dt = (float)(SDL_GetTicks() - lastTick) / 1000.0f;
        lastTick = SDL_GetTicks();

        activeScene->updateScene(dt);

        if (renderer != nullptr) {
          // @TODO create profiler helpers for this
          auto getTime = Gm_CreateTimer();

          renderer->render();

          auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(getTime()).count();

          log("Render time: ", microseconds, "us");
        }
      }
    }

    // Post-quit cleanup
    destroyRenderer();

    controller->destroy();

    delete controller;

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
        // @TODO
        break;
    }

    if (renderer != nullptr) {
      renderer->init();

      if (AbstractScene::active != nullptr) {
        // @TODO get all active meshes/lights and create them in the renderer instance
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
}