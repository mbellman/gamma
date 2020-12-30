#include "DemoScene.h"
#include "Gamma.h"

using namespace Gamma;

void DemoScene::init() {
  addMesh("cube", Gm_CreateCube(), 10);

  auto& cube = createObjectFrom("cube");

  cube.position = Vec3f(0.0f, 0.0f, 200.0f);
  cube.scale = 20.0f;
  cube.rotation = 0.0f;

  transform(cube);

  store("cube", cube);
}

void DemoScene::destroy() {
  // @TODO
}

void DemoScene::update(float dt) {
  Gamma::log("Delta:", dt);

  auto& cube = get("cube");

  cube.rotation.x += 0.5f * dt;
  cube.rotation.y += dt;
  cube.rotation.z += 0.84f * dt;

  transform(cube);
}