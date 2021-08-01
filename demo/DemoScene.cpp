#include <cmath>

#include "DemoScene.h"
#include "Gamma.h"

using namespace Gamma;

void DemoScene::init() {
  flags = SceneFlags::MODE_FREE_CAMERA;
  camera.position.z = -1000.0f;

  auto* cubeMesh = addMesh("cube", 100, Gm_CreateCube());

  cubeMesh->texture = "./demo/assets/images/cat.png";

  addMesh("rabbit", 10, Gm_LoadMesh("./demo/assets/models/rabbit.obj"));

  for (uint32 i = 0; i < 10; i++) {
    auto& rabbit = createObjectFrom("rabbit");
    float r = (float)i / 10.0f * M_PI * 2.0f;

    rabbit.scale = 50.0f;

    rabbit.position = Vec3f(
      sinf(r) * 300.0f,
      100.0f,
      cosf(r) * 300.0f
    );

    transform(rabbit);
  }

  auto& rabbitLight = createLight();

  rabbitLight.position = Vec3f(0.0f, 300.0f, 0.0f);
  rabbitLight.color = Vec3f(1.0f, 0.0f, 1.0f);
  rabbitLight.radius = 1000.0f;
  rabbitLight.power = 20.0f;

  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      auto& cube = createObjectFrom("cube");

      Vec3f cubePosition = Vec3f(
        200.0f * (i - 5),
        0.0f,
        200.0f * (j - 5)
      );

      cube.position = cubePosition;
      cube.scale = 30.0f;
      cube.rotation = Vec3f(1.3f, 0.9f, 2.2f);
      cube.rotation.y += (float)(i * 10 + j) / 5.0f;

      auto& light = createLight();

      light.color = Vec3f(
        i / 10.0f,
        sinf(i / 10.0f * 3.141592f) * sinf(j / 10.0f * 3.141592f),
        1.0f - j / 10.0f
      );

      light.position = cubePosition + Vec3f(0.0f, 50.0f, 0.0f);
      light.radius = 250.0f;
    }
  }

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

    if (key == Key::R) {
      getMeshObjects("cube").removeById(lastRemovedIndex++);
    }
  });
}

void DemoScene::destroy() {}

void DemoScene::update(float dt) {
  for (auto& cube : getMeshObjects("cube")) {
    cube.rotation.y += dt;

    transform(cube);
  }
}