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

  typedef std::function<void(float)> BehaviorHandler;

  struct ObjectRecord {
    uint32 meshId;
    uint32 meshGeneration;
    uint32 objectId;
    uint32 objectGeneration;
  };

  struct Behavior {
    ObjectRecord record;
    BehaviorHandler handler;
  };

  class AbstractScene : public Initable, public Destroyable, public Signaler {
  public:
    Camera camera;
    InputSystem input;
    uint32 flags = 0;

    static AbstractScene* active;

    virtual ~AbstractScene();

    virtual void addBehavior(const Object& object, BehaviorHandler handler);
    void addMesh(std::string name, Mesh* mesh, uint32 maxInstances);
    Light& createLight();
    Object& createObjectFrom(std::string name);
    const std::vector<Light>& getLights() const;
    void removeMesh(std::string name);
    void transform(const Object& object);
    virtual void update(float dt) {};
    virtual void updateScene(float dt) final;

  protected:
    Object& get(std::string);
    float getRunningTime();
    void store(std::string, Object& object);

  private:
    std::vector<Mesh*> meshes;
    std::map<std::string, Mesh*> meshMap;
    std::map<std::string, ObjectRecord> objectStore;
    std::vector<Light> lights;
    std::vector<Behavior> behaviors;
    float runningTime = 0.0f;

    Object* findObject(const ObjectRecord& record);
    void handleFreeCameraMode(float dt);
  };
}