#include <cstdio>

#include "DemoScene.h"
#include "Gamma.h"

void DemoScene::init() {
  using namespace Gamma;

  addMesh("cube", Gm_CreateCube());

  auto* cube = createObjectFrom("cube");

  cube->position(Vec3f(0.0f, 50.0f, 100.0f));
  cube->scale(20.0f);
}

void DemoScene::destroy() {

}

void DemoScene::update(float dt) {
  printf("Delta: %f\n", dt);
}