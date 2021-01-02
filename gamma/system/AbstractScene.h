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
  class AbstractScene : public Initable, public Destroyable, public Signaler {
  public:
    static AbstractScene* active;

    virtual ~AbstractScene();

    void addMesh(std::string name, Mesh* mesh, uint32 maxInstances);
    Light* createLight();
    Object& createObjectFrom(std::string name);
    void handleEvent(const SDL_Event& event);
    void removeMesh(std::string name);
    void transform(const Object& object);
    virtual void update(float dt) {};
    virtual void updateScene(float dt) final;

  protected:
    Camera camera;
    InputSystem input;

    Object& get(std::string);
    void store(std::string, Object& object);

  private:
    struct ObjectRecord {
      uint32 meshId;
      uint32 meshGeneration;
      uint32 objectId;
    };

    std::vector<Mesh*> meshes;
    std::map<std::string, Mesh*> meshMap;
    std::map<std::string, ObjectRecord> objectStore;
    std::vector<Light*> lights;
  };
}