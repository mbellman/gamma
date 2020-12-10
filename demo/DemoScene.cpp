#include <cstdio>

#include "DemoScene.h"

void DemoScene::init() {
  printf("DemoScene init\n");
}

void DemoScene::destroy() {

}

void DemoScene::update(float dt) {
  printf("Delta: %f\n", dt);
}