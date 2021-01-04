#include <cmath>

#include "DemoScene.h"
#include "Gamma.h"

using namespace Gamma;

void DemoScene::init() {
  flags = SceneFlags::MODE_FREE_CAMERA;

  addMesh("cube", Gm_CreateCube(), 100);

  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      auto& cube = createObjectFrom("cube");
      auto& light = createLight();

      Vec3f cubePosition = Vec3f(
        100.0f * (i - 5),
        0.0f,
        100.0f * (j - 5)
      );

      cube.position = cubePosition;
      cube.scale = 30.0f;
      cube.rotation = Vec3f(1.3f, 0.9f, 2.2f);

      light.color = Vec3f(
        i / 10.0f,
        sinf(i / 10.0f * 3.141592f) * sinf(j / 10.0f * 3.141592f),
        1.0f - j / 10.0f
      );

      light.position = cubePosition + Vec3f(-5.0f, 50.0f, 0.0f);
      light.radius = 250.0f;

      transform(cube);
    }
  }

  // store("cube", cube);

  input.on<MouseMoveEvent>("mousemove", [=](MouseMoveEvent& event) {
    if (SDL_GetRelativeMouseMode()) {
      camera.orientation.pitch += event.deltaY / 1000.0f;
      camera.orientation.yaw += event.deltaX / 1000.0f;
    }
  });

  input.on<MouseButtonEvent>("mousedown", [=](MouseButtonEvent& event) {
    if (!SDL_GetRelativeMouseMode()) {
      SDL_SetRelativeMouseMode(SDL_TRUE);
    }
  });

  input.on<Key>("keyup", [=](Key key) {
    if (key == Key::ESCAPE) {
      SDL_SetRelativeMouseMode(SDL_FALSE);
    }
  });
}

void DemoScene::destroy() {
  // @TODO
}

void DemoScene::update(float dt) {
  // auto& cube = get("cube");

  // cube.rotation.x += 0.5f * dt;
  // cube.rotation.y += dt;
  // cube.rotation.z += 0.84f * dt;

  // transform(cube);
}