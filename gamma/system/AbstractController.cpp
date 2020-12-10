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

    scene->onMeshCreated(handleMeshCreated);
    scene->onMeshDestroyed(handleMeshDestroyed);
    scene->init();
  }

  void AbstractController::leaveScene() {
    if (scenes.size() > 0) {
      destroyScene(scenes.back());

      scenes.pop_back();

      AbstractScene::active = scenes.size() > 0 ? scenes.back() : nullptr;
    }
  }

  void AbstractController::onMeshCreated(std::function<void(Mesh*)> handler) {
    // @TODO allow handler to fire retroactively for existing Meshes
    handleMeshCreated = handler;
  }

  void AbstractController::onMeshDestroyed(std::function<void(Mesh*)> handler) {
    handleMeshDestroyed = handler;
  }

  void AbstractController::switchScene(AbstractScene* scene) {
    if (scenes.size() > 0) {
      destroyScene(scenes.back());
      scenes.pop_back();
    }

    enterScene(scene);
  }
}