#include "system/AbstractController.h"
#include "system/AbstractScene.h"

namespace Gamma {
  AbstractController::~AbstractController() {
    for (auto* scene : scenes) {
      destroyScene(scene);
    }

    scenes.clear();
  }

  void AbstractController::destroyScene(AbstractScene* scene) {
    scene->destroy();

    delete scene;
  }

  void AbstractController::enterScene(AbstractScene* scene) {
    AbstractScene::active = scene;

    scenes.push_back(scene);

    // @todo change this to scene-changed, also signal in leaveScene()
    signal("scene-created", scene);

    scene->init();
  }

  void AbstractController::leaveScene() {
    if (scenes.size() > 0) {
      destroyScene(scenes.back());

      scenes.pop_back();

      AbstractScene::active = scenes.size() > 0 ? scenes.back() : nullptr;
    }
  }

  void AbstractController::switchScene(AbstractScene* scene) {
    if (scenes.size() > 0) {
      destroyScene(scenes.back());

      scenes.pop_back();
    }

    enterScene(scene);
  }
}