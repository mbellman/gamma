#include "system/AbstractController.h"
#include "system/AbstractScene.h"

namespace Gamma {
  void AbstractController::enterScene(AbstractScene* scene) {
    scenes.push_back(scene);
  }

  void AbstractController::leaveScene() {
    // @TODO delete scene
    scenes.pop_back();
  }

  void AbstractController::onMeshCreated(std::function<void(Mesh*)> handler) {
    handleMeshCreated = handler;
  }

  void AbstractController::onMeshDestroyed(std::function<void(Mesh*)> handler) {
    handleMeshDestroyed = handler;
  }

  void AbstractController::switchScene(AbstractScene* scene) {
    if (scenes.size() > 0) {
      // @TODO delete scene
      scenes.pop_back();
    }

    scenes.push_back(scene);
  }
}