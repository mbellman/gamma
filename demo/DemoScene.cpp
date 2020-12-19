#include "DemoScene.h"
#include "Gamma.h"

void DemoScene::init() {
  using namespace Gamma;

  addMesh("cube", Gm_CreateCube(), 10);

  auto& cube = createObjectFrom("cube");
  auto& cube2 = createObjectFrom("cube");

  cube.position = Vec3f(0.0f, 50.0f, 100.0f);
  cube.scale = 20.0f;

  transform(cube);
}

void DemoScene::destroy() {

}

void DemoScene::update(float dt) {
  Gamma::log("Delta:", dt);
}