#include "system/AbstractScene.h"

namespace Gamma {
  AbstractScene* AbstractScene::active = nullptr;

  Light* AbstractScene::createLight() {
    lights.push_back(new Light());

    return lights.back();
  }

  void AbstractScene::createMesh(std::string name, Mesh* mesh) {
    meshMap.emplace(name, mesh);

    signal("mesh-created", mesh);
  }

  Object* AbstractScene::createObjectFrom(std::string name) {
    // @TODO create Object record for associated Mesh
    return new Object();
  }

  void AbstractScene::removeMesh(std::string name) {
    if (meshMap.find(name) == meshMap.end()) {
      return;
    }

    auto* mesh = meshMap.at(name);

    signal("mesh-destroyed", mesh);
    Gm_FreeMesh(mesh);

    meshMap.erase(name);
  }
}