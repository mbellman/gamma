#include <cmath>

#include "DemoScene.h"
#include "Gamma.h"

using namespace Gamma;

void DemoScene::init() {
  flags = SceneFlags::MODE_FREE_CAMERA;
  camera.position.z = -1500.0f;

  auto* cubeMesh = addMesh("cube", 100, Gm_CreateCube());
  auto* rabbitMesh = addMesh("rabbit", 10, Gm_LoadMesh("./demo/assets/models/rabbit.obj"));
  auto* planeMesh = addMesh("plane", 1, Gm_CreatePlane(10));
  auto* wallMesh = addMesh("wall", 1, Gm_CreatePlane(10));

  cubeMesh->texture = "./demo/assets/images/cat.png";
  wallMesh->texture = "./demo/assets/images/cat.png";
  planeMesh->normalMap = "./demo/assets/images/metal-normal-map.png";
  planeMesh->isReflective = true;
  rabbitMesh->isReflective = true;

  auto& plane = createObjectFrom("plane");

  plane.scale = Vec3f(2000.0f, 1.0f, 2000.0f);
  plane.position.y = -100.0f;

  transform(plane);

  auto& wall = createObjectFrom("wall");

  wall.scale = Vec3f(2000.0f, 2000.0f, 2000.0f);
  wall.position = Vec3f(0.0f, 500.0f, 1100.0f);
  wall.rotation.x = M_PI * 0.5f;
  wall.rotation.y = M_PI;

  transform(wall);

  for (uint32 i = 0; i < 10; i++) {
    auto& rabbit = createObjectFrom("rabbit");
    float r = (float)i / 10.0f * M_PI * 2.0f;

    rabbit.scale = 50.0f;

    rabbit.position = Vec3f(
      sinf(r) * 300.0f,
      -85.0f,
      cosf(r) * 300.0f
    );

    transform(rabbit);
  }

  // auto& rabbitLight = createLight();

  // rabbitLight.position = Vec3f(0.f, 200.0f, 0.0f);
  // rabbitLight.color = Vec3f(1.0f, 0.0f, 1.0f);
  // rabbitLight.radius = 1000.0f;
  // rabbitLight.power = 20.0f;

  auto& sunlight = createLight();

  sunlight.type = LightType::DIRECTIONAL;
  sunlight.direction = Vec3f(-0.5, -1.0f, 1.0);
  sunlight.color = Vec3f(1.0f, 0.3f, 0.1f);

  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      auto& cube = createObjectFrom("cube");

      Vec3f cubePosition = Vec3f(
        200.0f * (i - 5),
        -70.0f,
        200.0f * (j - 5)
      );

      cube.position = cubePosition;
      cube.scale = 30.0f;
      // cube.rotation = Vec3f(1.3f, 0.9f, 2.2f);
      // cube.rotation.y += (float)(i * 10 + j) / 5.0f;

      transform(cube);
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
  // for (auto& cube : getMeshObjects("cube")) {
  //   cube.rotation.y += dt;
  //   cube.position.y = sinf(0.5f * (float)cube._record.id + getRunningTime() * 3.0f) * 20.0f;

  //   transform(cube);
  // }
}