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
  // @todo create/use global Gamma flags instead of tying
  // any particular features to scenes, renders, etc.
  enum SceneFlags {
    MODE_FREE_CAMERA = 1 << 0,
    MODE_MOVABLE_OBJECTS = 1 << 1,  // @todo
    MODE_WIREFRAME = 1 << 2,
    MODE_VSYNC = 1 << 3
  };

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

    uint32 getFlags() const;
    const std::vector<Light>& getLights() const;
    const SceneStats getStats() const;
    virtual void updateScene(float dt) final;

  protected:
    uint32 flags = 0;

    Mesh* addMesh(std::string name, uint16 maxInstances, Mesh* mesh);
    Light& createLight();
    Object& createObjectFrom(std::string meshName);
    void disableFlags(SceneFlags flags);
    ObjectPool& getMeshObjects(std::string meshName);
    Object& getObject(std::string name);
    float getRunningTime();
    void enableFlags(SceneFlags flags);
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