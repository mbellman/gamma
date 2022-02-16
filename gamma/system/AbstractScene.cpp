#include <filesystem>

#include "system/AbstractScene.h"
#include "system/assert.h"
#include "system/console.h"
#include "system/flags.h"
#include "system/yaml_parser.h"

namespace Gamma {
  /**
   * AbstractScene
   * -------------
   */
  AbstractScene* AbstractScene::active = nullptr;

  AbstractScene::~AbstractScene() {
    // @todo clear vectors + maps, free all resources
  }

  void AbstractScene::addMesh(const std::string& meshName, uint16 maxInstances, Mesh* mesh) {
    assert(meshMap.find(meshName) == meshMap.end(), "Mesh '" + meshName + "' already exists!");

    mesh->index = (uint16)meshes.size();
    mesh->id = runningMeshId++;
    mesh->objects.reserve(maxInstances);

    if (mesh->lods.size() > 0) {
      mesh->lods[0].instanceOffset = 0;
      mesh->lods[0].instanceCount = maxInstances;
    }

    meshMap.emplace(meshName, mesh);
    meshes.push_back(mesh);

    if (mesh->type == MeshType::PARTICLE_SYSTEM) {
      for (uint16 i = 0; i < maxInstances; i++) {
        createObjectFrom(meshName);
      }
    }

    signal("mesh-created", mesh);
  }

  void AbstractScene::addProbe(const std::string& probeName, const Vec3f& position) {
    probeMap.emplace(probeName, position);
  }

  void AbstractScene::commit(const Object& object) {
    auto& record = object._record;
    auto* mesh = meshes[record.meshIndex];

    // @todo (?) dispatch transform commands to separate buckets for multithreading
    mesh->objects.transformById(record.id, Matrix4f::transformation(
      object.position,
      object.scale,
      object.rotation
    ).transpose());

    mesh->objects.setColorById(record.id, object.color);
  }

  Light& AbstractScene::createLight(LightType type) {
    // @todo recycle removed/deactivated Lights
    // @todo new Light()
    lights.push_back(Light());

    auto& light = lights.back();

    light.type = type;

    if (
      type == LightType::POINT_SHADOWCASTER ||
      type == LightType::DIRECTIONAL_SHADOWCASTER ||
      type == LightType::SPOT_SHADOWCASTER
    ) {
      signal("shadowcaster-created", &light);
    }

    return light;
  }

  Object& AbstractScene::createObjectFrom(const std::string& meshName) {
    assert(meshMap.find(meshName) != meshMap.end(), "Mesh '" + meshName + "' not found");

    // @todo assert that mesh exists
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

  Object& AbstractScene::getObject(const std::string& name) {
    auto& record = objectStore.at(name);
    auto* object = findObject(record);

    assert(object != nullptr, "Object '" + name + "' no longer exists");

    return *object;
  }

  const std::map<std::string, Vec3f> AbstractScene::getProbeMap() const {
    return probeMap;
  }

  const float AbstractScene::getRunningTime() {
    return runningTime;
  }

  const SceneStats AbstractScene::getStats() const {
    SceneStats stats;

    for (auto* mesh : meshes) {
      if (mesh->lods.size() > 0) {
        for (auto& lod : mesh->lods) {
          stats.verts += lod.vertexCount * lod.instanceCount;
          stats.tris += (lod.elementCount / 3) * lod.instanceCount;
        }
      } else {
        stats.verts += mesh->vertices.size() * mesh->objects.totalVisible();
        stats.tris += (mesh->faceElements.size() / 3) * mesh->objects.totalVisible();
      }
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
      float speed = input.isKeyHeld(Key::SHIFT) ? 200.0f : 1000.0f;

      freeCameraVelocity += direction.unit() * speed * dt;
    }

    camera.position += freeCameraVelocity * dt;
    freeCameraVelocity *= (0.995f - dt * 5.0f);

    if (freeCameraVelocity.magnitude() < 0.1f) {
      freeCameraVelocity = Vec3f(0.0f);
    }
  }

  void AbstractScene::lookAt(const Object& object) {
    lookAt(object.position);
  }

  void AbstractScene::lookAt(const Vec3f& position) {
    Vec3f forward = position - camera.position;
    Vec3f sideways = Vec3f::cross(forward, Vec3f(0, 1.0f, 0));
    Vec3f up = Vec3f::cross(sideways, forward);

    camera.orientation.face(forward, up);
  }

  Mesh& AbstractScene::mesh(const std::string& meshName) {
    assert(meshMap.find(meshName) != meshMap.end(), "Mesh '" + meshName + "' does not exist!");

    return *meshMap[meshName];
  }

  void AbstractScene::removeMesh(const std::string& meshName) {
    if (meshMap.find(meshName) == meshMap.end()) {
      return;
    }

    auto* mesh = meshMap.at(meshName);

    signal("mesh-destroyed", mesh);

    Gm_FreeMesh(mesh);

    meshMap.erase(meshName);
    // @todo remove/reset mesh by ID, recycle when next mesh is created
  }

  void AbstractScene::storeObject(const std::string& name, Object& object) {
    objectStore.emplace(name, object._record);
  }

  void AbstractScene::updateScene(float dt) {
    if (Gm_IsFlagEnabled(GammaFlags::FREE_CAMERA_MODE)) {
      handleFreeCameraMode(dt);
    }

    update(dt);

    #if GAMMA_DEVELOPER_MODE
      // @todo extract into its own method
      for (auto& record : sceneFileRecords) {
        auto& fsPath = std::filesystem::current_path() / record.path;
        auto lastWriteTime = std::filesystem::last_write_time(fsPath);

        if (lastWriteTime != record.lastWriteTime) {
          auto& scene = Gm_ParseYamlFile(record.path.c_str());

          record.lastWriteTime = lastWriteTime;

          for (auto& [ key, property ] : *scene["meshes"].object) {
            if (meshMap.find(key) == meshMap.end()) {
              continue;
            }

            auto& meshConfig = *property.object;
            auto& mesh = meshMap.at(key);

            if (Gm_HasYamlProperty(meshConfig, "texture")) {
              mesh->texture = Gm_ReadYamlProperty<std::string>(meshConfig, "texture");
            } else {
              mesh->texture = "";
            }

            if (Gm_HasYamlProperty(meshConfig, "normalMap")) {
              mesh->normalMap = Gm_ReadYamlProperty<std::string>(meshConfig, "normalMap");
            } else {
              mesh->normalMap = "";
            }
          }
        }
      }
    #endif

    runningTime += dt;
  }

  // @todo define an initializer list overload for multiple meshes by name
  void AbstractScene::useFrustumCulling(Mesh& mesh) {
    mesh.objects.partitionByVisibility(camera);
  }

  void AbstractScene::useLodByDistance(Mesh& mesh, float distance) {
    uint32 instanceOffset = 0;

    for (uint32 lodIndex = 0; lodIndex < mesh.lods.size(); lodIndex++) {
      mesh.lods[lodIndex].instanceOffset = instanceOffset;

      if (lodIndex < mesh.lods.size() - 1) {
        // Group all objects within the distance threshold
        // in front of those outside it, and use the pivot
        // defining that boundary to determine our instance
        // count for this LoD set
        instanceOffset = (uint32)mesh.objects.partitionByDistance((uint16)instanceOffset, distance * float(lodIndex + 1), camera.position);

        mesh.lods[lodIndex].instanceCount = instanceOffset - mesh.lods[lodIndex].instanceOffset;
      } else {
        // The final LoD can just use the remaining set
        // of objects beyond the last LoD distance threshold
        mesh.lods[lodIndex].instanceCount = (uint32)mesh.objects.totalVisible() - instanceOffset;
      }
    }
  }

  void AbstractScene::useLodByDistance(float distance, const std::initializer_list<std::string>& meshNames) {
    for (uint32 i = 0; i < meshNames.size(); i++) {
      std::string meshName = *(meshNames.begin() + i);

      useLodByDistance(mesh(meshName), distance);
    }
  }

  /**
   * Takes a scene .yml file and loads its various mesh
   * and environmental detail configurations into the
   * current scene.
   */
  void AbstractScene::useSceneFile(const std::string& filename) {
    auto& scene = Gm_ParseYamlFile(filename.c_str());
    auto& fsPath = std::filesystem::current_path() / filename;
    auto lastWriteTime = std::filesystem::last_write_time(fsPath);

    sceneFileRecords.push_back({
      filename,
      lastWriteTime
    });

    // Load meshes
    for (auto& [ key, property ] : *scene["meshes"].object) {
      auto& meshConfig = *property.object;
      uint32 maxInstances = Gm_ReadYamlProperty<uint32>(meshConfig, "max");
      Mesh* mesh = nullptr;

      if (Gm_HasYamlProperty(meshConfig, "plane")) {
        uint32 size = Gm_ReadYamlProperty<uint32>(meshConfig, "plane.size");
        bool useLoopingTexture = Gm_ReadYamlProperty<uint32>(meshConfig, "plane.useLoopingTexture");

        mesh = Mesh::Plane(size, useLoopingTexture);
      } else if (Gm_HasYamlProperty(meshConfig, "cube")) {
        mesh = Mesh::Cube();
      } else if (Gm_HasYamlProperty(meshConfig, "model")) {
        std::vector<std::string> filepaths;
        auto& paths = Gm_ReadYamlProperty<YamlArray<std::string*>>(meshConfig, "model");

        for (auto* path : paths) {
          filepaths.push_back(*path);
        }

        mesh = Mesh::Model(filepaths);
      } else if (Gm_HasYamlProperty(meshConfig, "particles")) {
        // @todo
      }

      // if mesh == nullptr, report mesh name missing type

      if (mesh != nullptr) {
        if (Gm_HasYamlProperty(meshConfig, "texture")) {
          mesh->texture = Gm_ReadYamlProperty<std::string>(meshConfig, "texture");
        }

        if (Gm_HasYamlProperty(meshConfig, "normalMap")) {
          mesh->normalMap = Gm_ReadYamlProperty<std::string>(meshConfig, "normalMap");
        }

        if (Gm_HasYamlProperty(meshConfig, "type")) {
          std::string type = Gm_ReadYamlProperty<std::string>(meshConfig, "type");

          // @todo use a map
          if (type == "REFRACTIVE") {
            mesh->type = MeshType::REFRACTIVE;
          } else if (type == "REFLECTIVE") {
            mesh->type = MeshType::REFLECTIVE;
          } else if (type == "PROBE_REFLECTOR") {
            mesh->type = MeshType::PROBE_REFLECTOR;
            mesh->probe = Gm_ReadYamlProperty<std::string>(meshConfig, "probe");
          }
        }

        addMesh(key, maxInstances, mesh);
      }
    }

    // @todo skybox settings, what else?

    Gm_FreeYamlObject(&scene);
  }
}