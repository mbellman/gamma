#pragma once

#include <functional>
#include <vector>
#include <string>
#include <map>

#include "system/entities.h"
#include "system/Signaler.h"
#include "system/traits.h"

namespace Gamma {
  class AbstractScene : public Initable, public Destroyable, public Signaler2<Mesh*, Light*> {
  public:
    static AbstractScene* active;

    virtual ~AbstractScene() {};

    Light* createLight();
    void createMesh(std::string name, Mesh* mesh);
    Object* createObjectFrom(std::string name);
    void removeMesh(std::string name);
    virtual void update(float dt) {};

  private:
    std::map<std::string, Mesh*> meshMap;
    std::vector<Light*> lights;
  };
}