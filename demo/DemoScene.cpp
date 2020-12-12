#include <cstdio>

#include "DemoScene.h"
#include "Gamma.h"

void DemoScene::init() {
  using namespace Gamma;

  createMesh("cube", Gm_CreatePrimitiveMesh(Primitive::CUBE));

  auto* cube = createObjectFrom("cube");

  cube->position(Vec3f(1.0f));
  cube->scale(20.0f);
}

void DemoScene::destroy() {

}

void DemoScene::update(float dt) {
  // printf("Delta: %f\n", dt);
}