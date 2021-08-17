#pragma once

#include <functional>
#include <vector>
#include <string>
#include <map>

#include "system/camera.h"
#include "system/entities.h"
#include "system/InputSystem.h"
#include "system/Signaler.h"
#include "system/traits.h"
#include "system/type_aliases.h"

namespace Gamma {
  struct SceneStats {
    uint32 verts = 0;
    uint32 tris = 0;
  };

  class AbstractScene : public Initable, public Destroyable, public Signaler {
  public:
    Camera camera;
    InputSystem input;

    static AbstractScene* active;

    virtual ~AbstractScene();

    const std::vector<Light>& getLights() const;
    const SceneStats getStats() const;
    virtual void updateScene(float dt) final;

  protected:
    Mesh* addMesh(std::string name, uint16 maxInstances, Mesh* mesh);
    Light& createLight(LightType type);
    Object& createObjectFrom(std::string meshName);
    void destroyLight(Light& light);
    ObjectPool& getMeshObjects(std::string meshName);
    Object& getObject(std::string name);
    float getRunningTime();
    void removeMesh(std::string name);
    void transform(const Object& object);
    void storeObject(std::string, Object& object);
    virtual void update(float dt) {};

  private:
    std::vector<Mesh*> meshes;
    std::map<std::string, Mesh*> meshMap;
    std::map<std::string, ObjectRecord> objectStore;
    std::vector<Light> lights;
    float runningTime = 0.0f;
    Vec3f freeCameraVelocity = Vec3f(0.0f);
    uint16 runningMeshId = 0;

    Object* findObject(const ObjectRecord& record);
    void handleFreeCameraMode(float dt);
  };
}