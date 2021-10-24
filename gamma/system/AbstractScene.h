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
    const float getRunningTime();
    const SceneStats getStats() const;
    virtual void updateScene(float dt) final;

  protected:
    void addMesh(std::string meshName, uint16 maxInstances, Mesh* mesh);
    void addProbe(std::string probeName, const Vec3f& position);
    void commit(const Object& object);
    Light& createLight(LightType type);
    Object& createObjectFrom(std::string meshName);
    void destroyLight(Light& light);
    Object& getObject(std::string name);
    Mesh& mesh(std::string meshName);
    void removeMesh(std::string meshName);
    void storeObject(std::string, Object& object);
    virtual void update(float dt) {};
    void useLodByDistance(Mesh& mesh, float distance);

  private:
    std::vector<Mesh*> meshes;
    std::map<std::string, Mesh*> meshMap;
    std::map<std::string, Vec3f> probeMap;
    std::map<std::string, ObjectRecord> objectStore;
    std::vector<Light> lights;
    float runningTime = 0.0f;
    Vec3f freeCameraVelocity = Vec3f(0.0f);
    uint16 runningMeshId = 0;

    Object* findObject(const ObjectRecord& record);
    void handleFreeCameraMode(float dt);
  };
}