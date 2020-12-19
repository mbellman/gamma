#pragma once

#include <functional>
#include <vector>
#include <string>
#include <map>

#include "system/camera.h"
#include "system/entities.h"
#include "system/Signaler.h"
#include "system/traits.h"
#include "system/type_aliases.h"

namespace Gamma {
  struct TransformCommand {
    uint32 meshId;
    uint32 matrixId;
    uint32 objectId;
  };

  class AbstractScene : public Initable, public Destroyable, public Signaler {
  public:
    static AbstractScene* active;

    virtual ~AbstractScene();

    void addMesh(std::string name, Mesh* mesh, uint32 maxInstances);
    Light* createLight();
    Object& createObjectFrom(std::string name);
    void removeMesh(std::string name);
    void transform(const Object& object);
    virtual void update(float dt) {};
    virtual void updateScene(float dt) final;

  protected:
    Camera camera;

  private:
    std::vector<Mesh*> meshes;
    std::map<std::string, Mesh*> meshMap;
    std::vector<Light*> lights;
  };
}