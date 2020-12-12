#pragma once

#include <functional>
#include <vector>

#include "system/Signaler.h"
#include "system/traits.h"

namespace Gamma {
  struct Mesh;
  class AbstractScene;

  class AbstractController : public Initable, public Destroyable, public Signaler {
  public:
    virtual ~AbstractController();

  protected:
    void enterScene(AbstractScene* scene);
    void leaveScene();
    void switchScene(AbstractScene* scene);
  
  private:
    std::vector<AbstractScene*> scenes;

    void destroyScene(AbstractScene* scene);
  };
}