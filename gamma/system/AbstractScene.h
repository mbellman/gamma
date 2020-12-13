#pragma once

#include <functional>
#include <vector>
#include <string>
#include <map>

#include "system/camera.h"
#include "system/entities.h"
#include "system/Signaler.h"
#include "system/traits.h"

namespace Gamma {
  class AbstractScene : public Initable, public Destroyable, public Signaler {
  public:
    static AbstractScene* active;

    virtual ~AbstractScene();

    void addMesh(std::string name, Mesh* mesh);
    Light* createLight();
    Object* createObjectFrom(std::string name);
    void removeMesh(std::string name);
    virtual void update(float dt) {};
    virtual void updateScene(float dt) final;

  protected:
    Camera camera;

  private:
    std::map<std::string, Mesh*> meshMap;
    std::vector<Object*> objects;
    std::vector<Light*> lights;
  };
}