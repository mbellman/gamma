#pragma once

#include <functional>
#include <vector>

#include "system/traits.h"

namespace Gamma {
  struct Mesh;
  class AbstractScene;

  class AbstractController : public Initable, public Destroyable {
  public:
    virtual ~AbstractController();

    void onMeshCreated(std::function<void(Mesh*)> handler);
    void onMeshDestroyed(std::function<void(Mesh*)> handler);

  protected:
    void enterScene(AbstractScene* scene);
    void leaveScene();
    void switchScene(AbstractScene* scene);
  
  private:
    std::vector<AbstractScene*> scenes;
    std::function<void(Mesh*)> handleMeshCreated = nullptr;
    std::function<void(Mesh*)> handleMeshDestroyed = nullptr;

    void destroyScene(AbstractScene* scene);
  };
}