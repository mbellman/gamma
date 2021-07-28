#include "system/AbstractScene.h"
#include "system/assert.h"
#include "system/console.h"

namespace Gamma {
  /**
   * AbstractScene
   * -------------
   */
  AbstractScene* AbstractScene::active = nullptr;

  AbstractScene::~AbstractScene() {
    // @TODO clear vectors + maps, free all resources
  }

  // @TODO replace Mesh* with std::function<void(Mesh*)>, have handler
  // receive the recycled mesh and reconstruct its vertices/face indexes
  void AbstractScene::addMesh(std::string name, Mesh* mesh, uint16 maxInstances) {
    mesh->index = (uint16)meshes.size();
    mesh->id = runningMeshId++;
    mesh->objects.reserve(maxInstances);

    meshMap.emplace(name, mesh);
    meshes.push_back(mesh);

    signal("mesh-created", mesh);
  }

  Light& AbstractScene::createLight() {
    // @TODO how can we determine whether the light is a shadowcaster
    // and dispatch the appropriate signal?
    // @TODO recycle removed/deactivated Lights
    lights.push_back(Light());

    return lights.back();
  }

  Object& AbstractScene::createObjectFrom(std::string name) {
    auto& mesh = *meshMap.at(name);

    assert(
      !mesh.objects.isFull(),
      "Failed to create object: a maximum of " + std::to_string(mesh.objects.max()) + " object(s) allowed for mesh '" + name + "'"
    );

    auto& object = mesh.objects.createObject();

    object._record.meshId = mesh.id;
    object._record.meshIndex = mesh.index;
    object.position = Vec3f(0.0f);
    object.rotation = Vec3f(0.0f);
    object.scale = Vec3f(1.0f);

    return object;
  }

  Object* AbstractScene::findObject(const ObjectRecord& record) {
    auto& mesh = *meshes[record.meshIndex];

    if (mesh.id != record.meshId) {
      return nullptr;
    }

    auto& object = mesh.objects[record.index];

    if (object._record.id != record.id) {
      return nullptr;
    }

    return &object;
  }

  const std::vector<Light>& AbstractScene::getLights() const {
    return lights;
  }

  ObjectPool& AbstractScene::getMeshObjects(std::string name) {
    return meshMap[name]->objects;
  }

  Object& AbstractScene::getObject(std::string name) {
    auto& record = objectStore.at(name);
    auto* object = findObject(record);

    assert(object != nullptr, "Object '" + name + "' no longer exists");

    return *object;
  }

  float AbstractScene::getRunningTime() {
    return runningTime;
  }

  void AbstractScene::handleFreeCameraMode(float dt) {
    float speed = 150.0f * dt;
    Vec3f& position = camera.position;
    const Orientation& orientation = camera.orientation;

    if (input.isKeyHeld(Key::A)) {
      position += orientation.getLeftDirection() * speed;
    } else if (input.isKeyHeld(Key::D)) {
      position += orientation.getRightDirection() * speed;
    }

    if (input.isKeyHeld(Key::W)) {
      position += orientation.getDirection() * speed;
    } else if (input.isKeyHeld(Key::S)) {
      position += orientation.getDirection().invert() * speed;
    }
  }

  void AbstractScene::removeMesh(std::string name) {
    if (meshMap.find(name) == meshMap.end()) {
      return;
    }

    auto* mesh = meshMap.at(name);

    signal("mesh-destroyed", mesh);

    Gm_FreeMesh(mesh);

    meshMap.erase(name);
    // @TODO remove/reset mesh by ID, recycle when next mesh is created
  }

  void AbstractScene::storeObject(std::string name, Object& object) {
    objectStore.emplace(name, object._record);
  }

  void AbstractScene::transform(const Object& object) {
    auto& record = object._record;
    auto* mesh = meshes[record.meshId];

    // @TODO (?) dispatch transform commands to separate buckets for multithreading
    mesh->objects.transform(record.index, Matrix4f::transformation(
      object.position,
      object.scale,
      object.rotation
    ).transpose());
  }

  void AbstractScene::updateScene(float dt) {
    if (flags & SceneFlags::MODE_FREE_CAMERA) {
      handleFreeCameraMode(dt);
    }

    update(dt);

    runningTime += dt;
  }
}