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
  enum SceneFlags {
    MODE_FREE_CAMERA = 1 << 0,
    MODE_MOVABLE_OBJECTS = 1 << 1,  // @TODO
    MODE_WIREFRAME = 1 << 2
  };

  class AbstractScene : public Initable, public Destroyable, public Signaler {
  public:
    Camera camera;
    InputSystem input;
    uint32 flags = 0;

    static AbstractScene* active;

    virtual ~AbstractScene();

    void addMesh(std::string name, Mesh* mesh, uint16 maxInstances);
    Light& createLight();
    Object& createObjectFrom(std::string meshName);
    const std::vector<Light>& getLights() const;
    void removeMesh(std::string name);
    void transform(const Object& object);
    virtual void update(float dt) {};
    virtual void updateScene(float dt) final;

  protected:
    ObjectPool& getMeshObjects(std::string meshName);
    Object& getObject(std::string name);
    float getRunningTime();
    void storeObject(std::string, Object& object);

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