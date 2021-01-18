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

  void AbstractScene::addBehavior(const Object& object, BehaviorHandler handler) {
    ObjectRecord record;

    record.meshId = object._meshId;
    record.meshGeneration = object._meshGeneration;
    record.objectId = object._objectId;
    record.objectGeneration = object._generation;

    behaviors.push_back({ record, handler });
  }

  // @TODO replace Mesh* with std::function<void(Mesh*)>, have handler
  // receive the recycled mesh and reconstruct its vertices/face indexes
  void AbstractScene::addMesh(std::string name, Mesh* mesh, uint32 maxInstances) {
    // @TODO use recycled mesh index
    mesh->id = meshes.size();
    mesh->maxInstances = maxInstances;
    mesh->objects = new Object[maxInstances];
    mesh->matrices = new Matrix4f[maxInstances];

    meshMap.emplace(name, mesh);
    // @TDOO recycle meshes upon removal
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
      mesh.maxInstances > mesh.totalActiveObjects,
      "Failed to create object: a maximum of " + std::to_string(mesh.maxInstances) + " object(s) allowed for mesh '" + name + "'"
    );

    // @TODO recycle unused objects
    auto& object = mesh.objects[mesh.totalActiveObjects];

    object._meshId = mesh.id;
    object._meshGeneration = mesh.generation;
    object._objectId = mesh.totalActiveObjects++;
    object._matrixId = mesh.totalActiveMatrices++;
    object.position = Vec3f(0.0f);
    object.rotation = Vec3f(0.0f);
    object.scale = Vec3f(1.0f);

    return object;
  }

  // @TODO write a pool class for more efficient meshes/objects/etc. storage + lookup
  Object* AbstractScene::findObject(const ObjectRecord& record) {
    if (record.meshId >= meshes.size()) {
      return nullptr;
    }

    auto& mesh = *meshes[record.meshId];

    if (record.meshGeneration != mesh.generation) {
      return nullptr;
    }

    auto& object = mesh.objects[record.objectId];

    if (object._generation != record.objectGeneration) {
      return nullptr;
    }

    return &object;
  }

  Object& AbstractScene::get(std::string name) {
    auto& record = objectStore.at(name);
    auto* object = findObject(record);

    assert(object != nullptr, "Object '" + name + "' is invalid");

    return *object;
  }

  const std::vector<Light>& AbstractScene::getLights() const {
    return lights;
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

  void AbstractScene::store(std::string name, Object& object) {
    ObjectRecord record;

    record.meshId = object._meshId;
    record.meshGeneration = object._meshGeneration;
    record.objectId = object._objectId;

    objectStore.emplace(name, record);
  }

  void AbstractScene::transform(const Object& object) {
    // @TODO getMesh(uint32 id, uint32 generation)
    auto& mesh = *meshes[object._meshId];

    // @TODO dispatch transform commands to separate buckets for multithreading
    mesh.matrices[object._matrixId] = Matrix4f::transformation(
      object.position,
      object.scale,
      object.rotation
    ).transpose();
  }

  void AbstractScene::updateScene(float dt) {
    if (flags & SceneFlags::MODE_FREE_CAMERA) {
      handleFreeCameraMode(dt);
    }

    // Run updates on the subclassed scene
    update(dt);

    // Run behaviors
    for (uint32 i = 0; i < behaviors.size(); i++) {
      auto& behavior = behaviors[i];
      auto* object = findObject(behavior.record);

      if (object != nullptr) {
        behavior.handler(dt);
      }
    }

    runningTime += dt;
  }
}