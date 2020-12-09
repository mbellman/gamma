#include "DemoController.h"
#include "DemoScene.h"

void DemoController::init() {
  enterScene(new DemoScene());
}

void DemoController::destroy() {

}