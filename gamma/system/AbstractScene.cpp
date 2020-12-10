#include "system/AbstractScene.h"

namespace Gamma {
  AbstractScene* AbstractScene::active = nullptr;

  void AbstractScene::addMesh(std::string key, Mesh* mesh) {
    meshMap.emplace(key, mesh);

    if (handleMeshCreated != nullptr) {
      handleMeshCreated(mesh);
    }
  }

  void AbstractScene::onMeshCreated(std::function<void(Mesh*)> handler) {
    handleMeshCreated = handler;
  }

  void AbstractScene::onMeshDestroyed(std::function<void(Mesh*)> handler) {
    handleMeshDestroyed = handler;
  }

  void AbstractScene::removeMesh(std::string key) {
    if (meshMap.find(key) == meshMap.end()) {
      return;
    }

    if (handleMeshDestroyed != nullptr) {
      handleMeshDestroyed(meshMap.at(key));
    }

    meshMap.erase(key);
  }
}