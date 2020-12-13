#include "system/AbstractScene.h"

namespace Gamma {
  AbstractScene* AbstractScene::active = nullptr;

  AbstractScene::~AbstractScene() {
    // @TODO clear vectors + maps, free all resources
  }

  void AbstractScene::addMesh(std::string name, Mesh* mesh) {
    meshMap.emplace(name, mesh);

    signal("mesh-created", mesh);
  }

  Light* AbstractScene::createLight() {
    // @TODO how can we determine whether the light is a shadowcaster
    // and dispatch the appropriate signal?
    lights.push_back(new Light());

    return lights.back();
  }

  Object* AbstractScene::createObjectFrom(std::string name) {
    // @TODO create Object record for associated Mesh
    auto* object = new Object();

    objects.push_back(object);

    return object;
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

  void AbstractScene::updateScene(float dt) {
    Camera::active = &camera;

    // run updates on the subclassed scene
    update(dt);

    // run updates on objects, postponing loop incrementation
    // until we know whether an object was removed in its update
    for (uint32 i = 0; i < objects.size();) {
      auto& object = *objects[i];

      if (object.lifetime != -1) {
        object.lifetime -= (int)(1000.0f * dt);

        if (object.lifetime < 0) {
          object.lifetime = 0;
        }
      }

      if (object.lifetime == 0) {
        // @TODO Gm_FreeObject() -> deallocate + delete record from reference Mesh
        delete objects[i];

        objects.erase(objects.begin() + i);

        continue;
      }

      if (object._flags & ObjectFlags::IS_DIRTY) {
        Gm_RecomputeObjectMatrix(&object);
      }

      i++;
    }
  }
}