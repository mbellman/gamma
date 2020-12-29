#include "DemoScene.h"
#include "Gamma.h"

void DemoScene::init() {
  using namespace Gamma;

  addMesh("cube", Gm_CreateCube(), 10);

  auto& cube = createObjectFrom("cube");

  cube.position = Vec3f(0.0f, 0.0f, 150.0f);
  cube.scale = 20.0f;
  cube.rotation = Vec3f(0.5f, 0.9f, 2.3f);

  transform(cube);
}

void DemoScene::destroy() {

}

void DemoScene::update(float dt) {
  Gamma::log("Delta:", dt);
}