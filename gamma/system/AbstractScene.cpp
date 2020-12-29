#include "system/AbstractScene.h"
#include "system/assert.h"

namespace Gamma {
  AbstractScene* AbstractScene::active = nullptr;

  AbstractScene::~AbstractScene() {
    // @TODO clear vectors + maps, free all resources
  }

  void AbstractScene::addMesh(std::string name, Mesh* mesh, uint32 maxInstances) {
    mesh->id = meshes.size();
    mesh->maxInstances = maxInstances;
    mesh->objects = new Object[maxInstances];
    mesh->matrices = new Matrix4f[maxInstances];

    meshMap.emplace(name, mesh);
    meshes.push_back(mesh);

    signal("mesh-created", mesh);
  }

  Light* AbstractScene::createLight() {
    // @TODO how can we determine whether the light is a shadowcaster
    // and dispatch the appropriate signal?
    lights.push_back(new Light());

    return lights.back();
  }

  Object& AbstractScene::createObjectFrom(std::string name) {
    auto* mesh = meshMap.at(name);

    assert(
      mesh->maxInstances > mesh->totalActiveObjects + 1,
      "Failed to create object: a maximum of " + std::to_string(mesh->maxInstances) + " object(s) allowed for mesh '" + name + "'"
    );

    auto& object = mesh->objects[mesh->totalActiveObjects];

    object._meshId = mesh->id;
    object._objectId = mesh->totalActiveObjects++;
    object._matrixId = mesh->totalActiveMatrices++;

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

  void AbstractScene::transform(const Object& object) {
    auto& mesh = *meshes[object._meshId];

    // @TODO dispatch transform commands to separate buckets for multithreading
    mesh.matrices[object._matrixId] = Matrix4f::transformation(
      // @TODO make translation adjustments based on coordinate system handedness
      // what if render mode changes mid-game, with some objects already transformed?
      object.position * Vec3f(1.0f, 1.0f, -1.0f),
      object.scale,
      object.rotation
    ).transpose();
  }

  void AbstractScene::updateScene(float dt) {
    Camera::active = &camera;

    // run updates on the subclassed scene
    update(dt);
  }
}