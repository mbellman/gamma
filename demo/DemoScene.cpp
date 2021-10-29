#include <cmath>

#include "DemoScene.h"
#include "Gamma.h"

using namespace Gamma;

void DemoScene::init() {
  Gm_EnableFlags(GammaFlags::FREE_CAMERA_MODE);

  camera.position.z = -300.0f;

  addFloor();
  addCenterCubesExhibit();
  addRainbowCubesExhibit();

  // addMesh("daVinci", 1, Mesh::Model("./demo/assets/models/da-vinci.obj"));
  addMesh("wall", 1, Mesh::Plane(10));
  // addMesh("particles", 10000, Mesh::Particles());

  // addMesh("rabbit", 10, Mesh::Model({
  //   "./demo/assets/models/rabbit-lod1.obj",
  //   "./demo/assets/models/rabbit-lod2.obj"
  // }));

  // mesh("cube").texture = "./demo/assets/images/cat.png";
  // mesh("wall").texture = "./demo/assets/images/cat.png";
  // mesh("cube").texture = "./demo/assets/images/marble.png";
  // mesh("cube").normalMap = "./demo/assets/images/marble-normal-map.png";

  // mesh("plane").type = MeshType::REFLECTIVE;
  // mesh("rabbit").type = MeshType::REFRACTIVE;
  // mesh("cube").type = MeshType::EMISSIVE;

  // auto& particles = mesh("particles").particleSystem;

  // particles.spawn = Vec3f(0.0f, 20.0f, 0.0f);
  // particles.spread = 10.0f;
  // particles.minimumRadius = 0.0f;
  // particles.medianSpeed = 0.1f;
  // particles.speedVariation = 0.1f;
  // particles.medianSize = 10.0f;
  // particles.sizeVariation = 10.0f;
  // particles.deviation = 3.0f;
  // particles.isCircuit = true;

  // particles.path = {
  //   Vec3f(0.0f, 20.0f, 0.0f),
  //   Vec3f(20.0f, -10.0f, -40.0f),
  //   Vec3f(50.0f, 40.0f, 10.0f),
  //   Vec3f(0.0f, 30.0f, 10.0f),
  //   Vec3f(-20.0f, 40.0f, 35.0f),
  //   Vec3f(-40.0f, 15.0f, 25.0f),
  //   Vec3f(-60.0f, 20.0f, -30.0f),
  //   Vec3f(-5.0f, 20.0f, -5)
  // };

  // auto& daVinci = createObjectFrom("daVinci");

  // daVinci.position = Vec3f(0.0f, 20.0f, 0.0f);
  // daVinci.scale = 50.0f;
  // daVinci.color = pVec4(100, 175, 255);

  // commit(daVinci);

  auto& wall = createObjectFrom("wall");

  wall.scale = Vec3f(400.0f, 400.0f, 400.0f);
  wall.position = Vec3f(0.0f, 100.0f, 200.0f);
  wall.rotation.x = M_PI * 0.5f;
  wall.color = pVec4(255, 0, 0);

  commit(wall);

  // for (uint32 i = 0; i < 10; i++) {
  //   auto& rabbit = createObjectFrom("rabbit");
  //   float r = (float)i / 10.0f * M_PI * 2.0f;

  //   rabbit.scale = 10.0f;

  //   rabbit.position = Vec3f(
  //     sinf(r) * 60.0f,
  //     -17.0f,
  //     cosf(r) * 60.0f
  //   );

  //   rabbit.rotation = Vec3f(0.0f, -1.0f * i / 10.0f * 3.141592f * 2.0f, 0.0f);

  //   rabbit.color = pVec4(
  //     Vec3f(
  //       i / 10.0f,
  //       sinf(i / 10.0f * 3.141592f),
  //       1.0f - i / 10.0f
  //     )
  //   );

  //   commit(rabbit);
  // }

  // auto& daVinciLight = createLight(LightType::SPOT_SHADOWCASTER);

  // daVinciLight.position = Vec3f(0.0f, 20.0f, 0.0f);
  // daVinciLight.color = Vec3f(1.0f, 0.0f, 0.0f);
  // daVinciLight.radius = 1000.0f;
  // daVinciLight.power = 2.0f;
  // daVinciLight.direction = Vec3f(0.0f, 0.0f, 1.0f);
  // daVinciLight.fov = 80.0f;
  // daVinciLight.isStatic = true;

  // auto& daVinciLight2 = createLight(LightType::SPOT_SHADOWCASTER);

  // daVinciLight2.position = Vec3f(-50.0f, 20.0f, -50.0f);
  // daVinciLight2.color = Vec3f(0.0f, 1.0f, 0.0f);
  // daVinciLight2.radius = 1000.0f;
  // daVinciLight2.power = 2.0f;
  // daVinciLight2.direction = Vec3f(-0.3f, -1.0f, -0.3f);
  // daVinciLight2.fov = 80.0f;
  // daVinciLight2.isStatic = true;

  // auto& daVinciLight3 = createLight(LightType::SPOT_SHADOWCASTER);

  // daVinciLight3.position = Vec3f(50.0f, 20.0f, -50.0f);
  // daVinciLight3.color = Vec3f(0.0f, 0.0f, 1.0f);
  // daVinciLight3.radius = 1000.0f;
  // daVinciLight3.power = 2.0f;
  // daVinciLight3.direction = Vec3f(0.3f, -1.0f, -0.3f);
  // daVinciLight3.fov = 80.0f;
  // daVinciLight3.isStatic = true;

  auto& sunlight = createLight(LightType::DIRECTIONAL_SHADOWCASTER);

  sunlight.direction = Vec3f(-0.3f, -0.5f, 1.0f);
  sunlight.color = Vec3f(1.0f);

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
  // useLodByDistance(mesh("rabbit"), 100.0f);

  for (auto& cube : mesh("cat-cube").objects) {
    cube.rotation += Vec3f(0.5f * dt);

    commit(cube);
  }

  // auto& wall = *mesh("wall").objects.begin();

  // uint8 g = uint8((0.5f * sinf(getRunningTime() * 3.0f) + 0.5f) * 1.0f);
  // uint8 b = uint8((0.5f * cosf(getRunningTime() * 2.0f) + 0.5f) * 255.0f);

  // wall.color = pVec4(255, g, b);
  // wall.position.z = 0.0f + (1.0f + sinf(getRunningTime() * 0.2f)) * 0.5f * 200.0f;

  // commit(wall);
}

void DemoScene::addFloor() {
  addMesh("floor", 1, Mesh::Plane(30, true));

  mesh("floor").texture = "./demo/assets/images/checkerboard.png";
  mesh("floor").normalMap = "./demo/assets/images/marble-normal-map.png";

  auto& floor = createObjectFrom("floor");

  floor.scale = Vec3f(1000.0f, 1.0f, 1000.0f);
  floor.position.y = -20.0f;

  commit(floor);
}

void DemoScene::addCenterCubesExhibit() {
  Vec3f location(0.0f);

  addMesh("cat-cube", 3, Mesh::Cube());

  mesh("cat-cube").texture = "./demo/assets/images/cat.png";

  auto& cube1 = createObjectFrom("cat-cube");
  auto& cube2 = createObjectFrom("cat-cube");
  auto& cube3 = createObjectFrom("cat-cube");

  cube1.position = location + Vec3f(-50.0f, 15.0f, 0.0f);
  cube1.scale = 20.0f;
  cube1.rotation = Vec3f(1.5f, 0.7f, 2.1f);
  cube1.color = pVec4(255, 50, 10);

  cube2.position = location + Vec3f(0.0f, 15.0f, 0.0f);
  cube2.scale = 10.0f;
  cube2.rotation = Vec3f(0.3f, 1.1f, 0.8f);
  cube2.color = pVec4(10, 255, 50);

  cube3.position = location + Vec3f(30.0f, 15.0f, 0.0f);
  cube3.scale = 5.0f;
  cube3.rotation = Vec3f(0.9f, 2.5f, 3.1f);
  cube3.color = pVec4(10, 50, 255);

  commit(cube1);
  commit(cube2);
  commit(cube3);
}

void DemoScene::addRainbowCubesExhibit() {
  const float pi = 3.141592f;
  Vec3f location = Vec3f(-150.0f, 0.0f, 12.0f);

  addMesh("cube", 16, Mesh::Cube());
  addMesh("ball", 1, Mesh::Model("./demo/assets/models/ball.obj"));

  mesh("cube").texture = "./demo/assets/images/marble.png";
  mesh("cube").normalMap = "./demo/assets/images/marble-normal-map.png";

  mesh("ball").type = MeshType::PROBE_REFLECTOR;
  mesh("ball").probe = "ball-probe";

  auto& ball = createObjectFrom("ball");

  ball.scale = 16.0f;
  ball.position = location + Vec3f(-10.0f, 15.0f, -10.0f); 

  commit(ball);

  addProbe("ball-probe", ball.position);

  auto n_sinf = [](float value) {
    return sinf(value) * 0.5f + 0.5f;
  };

  auto n_cosf = [](float value) {
    return cosf(value) * 0.5f + 0.5f;
  };

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      auto& cube = createObjectFrom("cube");

      cube.position = location + Vec3f(
        20.0f * (i - 2.0f),
        -12.0f,
        20.0f * (j - 2.0f)
      );

      cube.scale = 8.0f;

      cube.color = pVec4(
        Vec3f(
          n_sinf(i / 3.0f * pi - pi * 0.5f),
          n_cosf(i / 3.0f * pi),
          n_sinf(j / 3.0f * pi - pi * 0.5f)
        )
      );

      commit(cube);
    }
  }
}