#pragma once

#include <functional>
#include <vector>
#include <string>
#include <map>

#include "system/object.h"
#include "system/traits.h"

namespace Gamma {
  class AbstractScene : public Initable, public Destroyable {
  public:
    static AbstractScene* active;

    virtual ~AbstractScene() {};

    void addMesh(std::string key, Mesh* mesh);
    void onMeshCreated(std::function<void(Mesh*)> handler);
    void onMeshDestroyed(std::function<void(Mesh*)> handler);
    void removeMesh(std::string key);
    virtual void update(float dt) {};

  private:
    std::function<void(Mesh*)> handleMeshCreated = nullptr;
    std::function<void(Mesh*)> handleMeshDestroyed = nullptr;
    std::map<std::string, Mesh*> meshMap;
  };
}