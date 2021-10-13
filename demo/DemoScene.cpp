#include <cmath>

#include "DemoScene.h"
#include "Gamma.h"

using namespace Gamma;

void DemoScene::init() {
  Gm_EnableFlags(GammaFlags::FREE_CAMERA_MODE);

  camera.position.z = -300.0f;

  addMesh("cube", 100, Mesh::Cube());
  addMesh("daVinci", 1, Mesh::Model("./demo/assets/models/da-vinci-split.obj"));
  addMesh("plane", 1, Mesh::Plane(10));
  addMesh("wall", 1, Mesh::Plane(10));
  addMesh("particles", 10000, Mesh::Particles());

  addMesh("rabbit", 10, Mesh::Model({
    "./demo/assets/models/rabbit-lod1.obj",
    "./demo/assets/models/rabbit-lod2.obj"
  }));

  // mesh("cube").texture = "./demo/assets/images/cat.png";
  // mesh("wall").texture = "./demo/assets/images/cat.png";
  mesh("plane").normalMap = "./demo/assets/images/metal-normal-map.png";

  auto& particles = mesh("particles").particleSystem;

  particles.spawn = Vec3f(0.0f, 20.0f, 0.0f);
  particles.spread = 10.0f;
  particles.minimumRadius = 0.0f;
  particles.medianSpeed = 0.1f;
  particles.speedVariation = 0.1f;
  particles.medianSize = 10.0f;
  particles.sizeVariation = 10.0f;
  particles.deviation = 3.0f;
  particles.isCircuit = true;

  particles.path = {
    Vec3f(0.0f, 20.0f, 0.0f),
    Vec3f(20.0f, -10.0f, -40.0f),
    Vec3f(50.0f, 40.0f, 10.0f),
    Vec3f(0.0f, 30.0f, 10.0f),
    Vec3f(-20.0f, 40.0f, 35.0f),
    Vec3f(-40.0f, 15.0f, 25.0f),
    Vec3f(-60.0f, 20.0f, -30.0f),
    Vec3f(-5.0f, 20.0f, -5)
  };

  // mesh("plane").type = MeshType::REFLECTIVE;
  // mesh("daVinci").type = MeshType::REFRACTIVE;

  mesh("rabbit").type = MeshType::EMISSIVE;

  auto& daVinci = createObjectFrom("daVinci");

  daVinci.position = Vec3f(0.0f, 20.0f, 0.0f);
  daVinci.scale = 50.0f;
  daVinci.color = pVec4(10, 20, 255);

  commit(daVinci);

  auto& plane = createObjectFrom("plane");

  plane.scale = Vec3f(400.0f, 1.0f, 400.0f);
  plane.position.y = -20.0f;

  commit(plane);

  auto& wall = createObjectFrom("wall");

  wall.scale = Vec3f(400.0f, 400.0f, 400.0f);
  wall.position = Vec3f(0.0f, 100.0f, 200.0f);
  wall.rotation.x = M_PI * 0.5f;
  wall.color = pVec4(255, 0, 0);

  commit(wall);

  for (uint32 i = 0; i < 10; i++) {
    auto& rabbit = createObjectFrom("rabbit");
    float r = (float)i / 10.0f * M_PI * 2.0f;

    rabbit.scale = 10.0f;

    rabbit.position = Vec3f(
      sinf(r) * 60.0f,
      -17.0f,
      cosf(r) * 60.0f
    );

      rabbit.color = pVec4(
        Vec3f(
          i / 10.0f,
          sinf(i / 10.0f * 3.141592f),
          1.0f - i / 10.0f
        )
      );

    // rabbit.color = pVec4(255, 200, 255);

    commit(rabbit);
  }

  // auto& daVinciLight = createLight(LightType::POINT_SHADOWCASTER);

  // daVinciLight.position = Vec3f(0.0f, 20.0f, 60.0f);
  // daVinciLight.color = Vec3f(1.0f, 1.0f, 1.0f);
  // daVinciLight.radius = 1000.0f;
  // daVinciLight.power = 2.0f;
  // daVinciLight.direction = Vec3f(0.0f, -1.0f, 0.5f);
  // daVinciLight.fov = 80.0f;
  // daVinciLight.isStatic = true;

  // auto& daVinciLight2 = createLight(LightType::POINT_SHADOWCASTER);

  // daVinciLight2.position = Vec3f(-50.0f, 20.0f, -50.0f);
  // daVinciLight2.color = Vec3f(0.0f, 1.0f, 0.0f);
  // daVinciLight2.radius = 1000.0f;
  // daVinciLight2.power = 2.0f;
  // daVinciLight2.direction = Vec3f(-0.3f, -1.0f, -0.3f);
  // daVinciLight2.fov = 80.0f;
  // daVinciLight2.isStatic = true;

  // auto& daVinciLight3 = createLight(LightType::POINT_SHADOWCASTER);

  // daVinciLight3.position = Vec3f(50.0f, 20.0f, -50.0f);
  // daVinciLight3.color = Vec3f(0.0f, 0.0f, 1.0f);
  // daVinciLight3.radius = 1000.0f;
  // daVinciLight3.power = 2.0f;
  // daVinciLight3.direction = Vec3f(0.3f, -1.0f, -0.3f);
  // daVinciLight3.fov = 80.0f;
  // daVinciLight3.isStatic = true;

  // auto& sunlight = createLight(LightType::DIRECTIONAL_SHADOWCASTER);

  // sunlight.direction = Vec3f(-0.3f, -0.5f, 1.0f);
  // // sunlight.color = Vec3f(1.0f, 0.3f, 0.1f);
  // sunlight.color = Vec3f(1.0f);

  auto& cameraLight = createLight(LightType::SPOT_SHADOWCASTER);

  cameraLight.color = Vec3f(1.0f, 1.0f, 1.0f);
  cameraLight.fov = 90.0f;
  cameraLight.radius = 500.0f;
  cameraLight.position = Vec3f(0.0f, 25.0f, 0.0f);
  cameraLight.direction = Vec3f(0.0f, 0.0f, 1.0f);
  cameraLight.power = 3.0f;

  clight = &cameraLight;

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

      cube.color = pVec4(
        Vec3f(
          i / 10.0f,
          sinf(i / 10.0f * 3.141592f) * sinf(j / 10.0f * 3.141592f),
          1.0f - j / 10.0f
        )
      );

      // auto& cubeLight = createLight(LightType::POINT);

      // cubeLight.color = Vec3f(0.1f, 0.3f, 1.0f);
      // cubeLight.radius = 25.0f;
      // cubeLight.position = cubePosition + Vec3f(0.0f, 10.0f, 0.0f);

      commit(cube);
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

  // auto& wall = *mesh("wall").objects.begin();

  // uint8 g = uint8((0.5f * sinf(getRunningTime()) + 0.5f) * 128.0f);
  // uint8 b = uint8((0.5f * cosf(getRunningTime()) + 0.5f) * 128.0f);

  // wall.color = pVec4(255, g, b);
  // wall.position.z = 0.0f + (1.0f + sinf(getRunningTime() * 0.2f)) * 0.5f * 200.0f;

  // commit(wall);

  clight->direction = camera.orientation.getDirection();
  clight->position = camera.position + camera.orientation.getDirection() * 20.0f + camera.orientation.getUpDirection() * -5.0f;
}