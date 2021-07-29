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

  Object& AbstractScene::createObjectFrom(std::string meshName) {
    auto& mesh = *meshMap.at(meshName);
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

    return mesh.objects.getByRecord(record);
  }

  const std::vector<Light>& AbstractScene::getLights() const {
    return lights;
  }

  ObjectPool& AbstractScene::getMeshObjects(std::string meshName) {
    return meshMap[meshName]->objects;
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
    const Orientation& orientation = camera.orientation;
    Vec3f direction;

    if (input.isKeyHeld(Key::A)) {
      direction += orientation.getLeftDirection();
    } else if (input.isKeyHeld(Key::D)) {
      direction += orientation.getRightDirection();
    }

    if (input.isKeyHeld(Key::W)) {
      direction += orientation.getDirection();
    } else if (input.isKeyHeld(Key::S)) {
      direction += orientation.getDirection().invert();
    }

    if (direction.magnitude() > 0.0f) {
      freeCameraVelocity += direction.unit() * 5000.0f * dt;
    }

    camera.position += freeCameraVelocity * dt;
    freeCameraVelocity *= (1.0f - dt * 5.0f);
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
    auto* mesh = meshes[record.meshIndex];

    // @TODO (?) dispatch transform commands to separate buckets for multithreading
    mesh->objects.transformById(record.id, Matrix4f::transformation(
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