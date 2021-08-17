#include "system/AbstractScene.h"
#include "system/assert.h"
#include "system/console.h"
#include "system/flags.h"

namespace Gamma {
  /**
   * AbstractScene
   * -------------
   */
  AbstractScene* AbstractScene::active = nullptr;

  AbstractScene::~AbstractScene() {
    // @todo clear vectors + maps, free all resources
  }

  Mesh* AbstractScene::addMesh(std::string name, uint16 maxInstances, Mesh* mesh) {
    assert(meshMap.find(name) == meshMap.end(), "Mesh '" + name + "' already exists!");

    mesh->index = (uint16)meshes.size();
    mesh->id = runningMeshId++;
    mesh->objects.reserve(maxInstances);

    meshMap.emplace(name, mesh);
    meshes.push_back(mesh);

    signal("mesh-created", mesh);

    return mesh;
  }

  Light& AbstractScene::createLight(LightType type) {
    // @todo recycle removed/deactivated Lights
    lights.push_back(Light());

    auto& light = lights.back();

    light.type = type;

    if (
      type == LightType::POINT_SHADOWCASTER ||
      type == LightType::DIRECTIONAL_SHADOWCASTER ||
      type == LightType::SPOT_SHADOWCASTER
    ) {
      signal("shadowcaster-created", light);
    }

    return light;
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

  void AbstractScene::destroyLight(Light& light) {
    // @todo reset light ID; signal when shadowcaster
    // lights are destroyed
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
    assert(meshMap.find(meshName) != meshMap.end(), "Mesh '" + meshName + "' does not exist!");

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

  const SceneStats AbstractScene::getStats() const {
    SceneStats stats;

    for (auto* mesh : meshes) {
      stats.verts += mesh->vertices.size() * mesh->objects.total();
      stats.tris += (mesh->faceElements.size() / 3) * mesh->objects.total();
    }

    return stats;
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
      float speed = input.isKeyHeld(Key::SHIFT) ? 1000.0f : 5000.0f;

      freeCameraVelocity += direction.unit() * speed * dt;
    }

    camera.position += freeCameraVelocity * dt;
    freeCameraVelocity *= (0.995f - dt * 5.0f);
  }

  void AbstractScene::removeMesh(std::string name) {
    if (meshMap.find(name) == meshMap.end()) {
      return;
    }

    auto* mesh = meshMap.at(name);

    signal("mesh-destroyed", mesh);

    Gm_FreeMesh(mesh);

    meshMap.erase(name);
    // @todo remove/reset mesh by ID, recycle when next mesh is created
  }

  void AbstractScene::storeObject(std::string name, Object& object) {
    objectStore.emplace(name, object._record);
  }

  void AbstractScene::transform(const Object& object) {
    auto& record = object._record;
    auto* mesh = meshes[record.meshIndex];

    // @todo (?) dispatch transform commands to separate buckets for multithreading
    mesh->objects.transformById(record.id, Matrix4f::transformation(
      object.position,
      object.scale,
      object.rotation
    ).transpose());
  }

  void AbstractScene::updateScene(float dt) {
    if (Gm_GetFlags() & GammaFlags::FREE_CAMERA_MODE) {
      handleFreeCameraMode(dt);
    }

    update(dt);

    runningTime += dt;
  }
}