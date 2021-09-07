#include <cmath>

#include "DemoScene.h"
#include "Gamma.h"

using namespace Gamma;

void DemoScene::init() {
  Gm_EnableFlags(GammaFlags::FREE_CAMERA_MODE);

  camera.position.z = -300.0f;

  addMesh("cube", 100, Gm_CreateCube());
  addMesh("daVinci", 1, Gm_LoadMesh("./demo/assets/models/da-vinci-split.obj"));
  addMesh("plane", 1, Gm_CreatePlane(10));
  addMesh("wall", 1, Gm_CreatePlane(10));

  addMesh("rabbit", 10, Gm_LoadMesh({
    "./demo/assets/models/rabbit-lod1.obj",
    "./demo/assets/models/rabbit-lod2.obj"
  }));

  mesh("cube").texture = "./demo/assets/images/cat.png";
  mesh("wall").texture = "./demo/assets/images/cat.png";
  mesh("plane").normalMap = "./demo/assets/images/metal-normal-map.png";
  // mesh("plane").type = MeshType::REFLECTIVE;
  // mesh("daVinci").type = MeshType::REFRACTIVE;

  auto& daVinci = createObjectFrom("daVinci");

  daVinci.position = Vec3f(0.0f, 20.0f, 0.0f);
  daVinci.scale = 50.0f;

  transform(daVinci);

  auto& plane = createObjectFrom("plane");

  plane.scale = Vec3f(400.0f, 1.0f, 400.0f);
  plane.position.y = -20.0f;

  transform(plane);

  auto& wall = createObjectFrom("wall");

  wall.scale = Vec3f(400.0f, 400.0f, 400.0f);
  wall.position = Vec3f(0.0f, 100.0f, 220.0f);
  wall.rotation.x = M_PI * 0.5f;

  transform(wall);

  for (uint32 i = 0; i < 10; i++) {
    auto& rabbit = createObjectFrom("rabbit");
    float r = (float)i / 10.0f * M_PI * 2.0f;

    rabbit.scale = 10.0f;

    rabbit.position = Vec3f(
      sinf(r) * 60.0f,
      -17.0f,
      cosf(r) * 60.0f
    );

    transform(rabbit);
  }

  // auto& daVinciLight = createLight(LightType::POINT_SHADOWCASTER);
  auto& daVinciLight = createLight(LightType::POINT_SHADOWCASTER);

  daVinciLight.position = Vec3f(0.0f, 20.0f, 0.0f);
  daVinciLight.color = Vec3f(1.0f, 0.0f, 1.0f);
  daVinciLight.radius = 1000.0f;
  daVinciLight.power = 2.0f;
  daVinciLight.isStatic = true;

  // auto& sunlight = createLight(LightType::DIRECTIONAL_SHADOWCASTER);

  // sunlight.direction = Vec3f(-0.3f, -0.5f, 1.0f);
  // sunlight.color = Vec3f(1.0f, 0.3f, 0.1f);

  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      auto& cube = createObjectFrom("cube");

      Vec3f cubePosition = Vec3f(
        40.0f * (i - 5),
        -12.0f,
        40.0f * (j - 5)
      );

      cube.position = cubePosition;
      cube.scale = 8.0f;

      // auto& cubeLight = createLight(LightType::POINT);

      // cubeLight.color = Vec3f(0.1f, 0.3f, 1.0f);
      // cubeLight.radius = 25.0f;
      // cubeLight.position = cubePosition + Vec3f(0.0f, 10.0f, 0.0f);

      transform(cube);
    }
  }

  input.on<MouseMoveEvent>("mousemove", [&](const MouseMoveEvent& event) {
    if (SDL_GetRelativeMouseMode()) {
      camera.orientation.pitch += event.deltaY / 1000.0f;
      camera.orientation.yaw += event.deltaX / 1000.0f;
    }
  });

  input.on<MouseButtonEvent>("mousedown", [&](const MouseButtonEvent& event) {
    if (!SDL_GetRelativeMouseMode()) {
      SDL_SetRelativeMouseMode(SDL_TRUE);
    }
  });

  input.on<Key>("keyup", [&](Key key) {
    if (key == Key::ESCAPE) {
      SDL_SetRelativeMouseMode(SDL_FALSE);
    }

    if (key == Key::R) {
      mesh("cube").objects.removeById(lastRemovedIndex++);
    }

    if (key == Key::V) {
      if (Gm_IsFlagEnabled(GammaFlags::VSYNC)) {
        Gm_DisableFlags(GammaFlags::VSYNC);
      } else {
        Gm_EnableFlags(GammaFlags::VSYNC);
      }
    }
  });
}

void DemoScene::destroy() {}

void DemoScene::update(float dt) {
  useLodByDistance(mesh("rabbit"), 100.0f);

  // for (auto& cube : getMeshObjects("cube")) {
  //   cube.rotation.y += dt;
  //   cube.position.y = sinf(0.5f * (float)cube._record.id + getRunningTime() * 3.0f) * 20.0f;

  //   transform(cube);
  // }
}