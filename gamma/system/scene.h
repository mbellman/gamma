#pragma once

#include <filesystem>
#include <functional>
#include <initializer_list>
#include <map>
#include <string>
#include <vector>

#include "system/camera.h"
#include "system/entities.h"
#include "system/InputSystem.h"
#include "system/Signaler.h"
#include "system/traits.h"
#include "system/type_aliases.h"

// #define addMesh(meshName, maxInstances, mesh) Gm_AddMesh(context, meshName, maxInstances, mesh);
// #define addProbe(probeName, position) Gm_AddProbe(context, probeName, position);
// #define createObjectFrom(meshName) Gm_CreateObjectFrom(context, meshName);

struct GmContext;

struct GmSceneStats {
  Gamma::uint32 verts = 0;
  Gamma::uint32 tris = 0;
};

struct GmScene {
  Gamma::Camera camera;
  Gamma::InputSystem input;
  std::vector<Gamma::Mesh*> meshes;
  std::vector<Gamma::Light> lights;
  std::map<std::string, Gamma::Mesh*> meshMap;
  std::map<std::string, Gamma::Vec3f> probeMap;
  std::map<std::string, Gamma::ObjectRecord> objectStore;
  Gamma::Vec3f freeCameraVelocity = Gamma::Vec3f(0.0f);
  Gamma::uint16 runningMeshId = 0;
  Gamma::uint32 frame = 0;
  float runningTime = 0.0f;
};

const GmSceneStats Gm_GetSceneStats(GmContext* context);
void Gm_AddMesh(GmContext* context, const std::string& meshName, Gamma::uint16 maxInstances, Gamma::Mesh* mesh);
void Gm_AddProbe(GmContext* context, const std::string& probeName, const Gamma::Vec3f& position);
Gamma::Object& Gm_CreateObjectFrom(GmContext* context, const std::string& meshName);
void Gm_Commit(GmContext* context, const Gamma::Object& object);
void Gm_HandleFreeCameraMode(GmContext* context, float dt);

namespace Gamma {
  struct SceneStats {
    uint32 verts = 0;
    uint32 tris = 0;
  };

  struct SceneFileRecord {
    std::string path;
    std::filesystem::file_time_type lastWriteTime;
  };

  class AbstractScene : public Initable, public Destroyable, public Signaler {
  public:
    Camera camera;
    InputSystem input;
    std::vector<Mesh*> meshes;
    std::vector<Light> lights;

    static AbstractScene* active;

    virtual ~AbstractScene();

    const uint32 getFrame() const;
    const std::vector<Light>& getLights() const;
    const std::map<std::string, Vec3f> getProbeMap() const;
    const float getRunningTime();
    const SceneStats getStats() const;
    virtual void updateScene(float dt) final;

  protected:
    void addMesh(const std::string& meshName, uint16 maxInstances, Mesh* mesh);
    void addProbe(const std::string& probeName, const Vec3f& position);
    void commit(const Object& object);
    Light& createLight(LightType type);
    Object& createObjectFrom(const std::string& meshName);
    void destroyLight(Light& light);
    Object& getObject(const std::string& name);
    void lookAt(const Object& object, bool upsideDown = false);
    void lookAt(const Vec3f& position, bool upsideDown = false);
    Mesh& mesh(const std::string& meshName);
    void removeMesh(const std::string& meshName);
    void storeObject(const std::string&, Object& object);
    virtual void update(float dt) {};
    void useFrustumCulling(Mesh& mesh);
    void useLodByDistance(Mesh& mesh, float distance);
    void useLodByDistance(float distance, const std::initializer_list<std::string>& meshNames);
    void useSceneFile(const std::string& filename);

  private:
    std::map<std::string, Mesh*> meshMap;
    std::map<std::string, Vec3f> probeMap;
    std::map<std::string, ObjectRecord> objectStore;
    std::vector<SceneFileRecord> sceneFileRecords;
    float runningTime = 0.0f;
    Vec3f freeCameraVelocity = Vec3f(0.0f);
    uint16 runningMeshId = 0;
    uint32 frame = 0;

    Object* findObject(const ObjectRecord& record);
    void handleFreeCameraMode(float dt);
  };
}