#pragma once

#include <vector>

#include "system/object.h"
#include "system/traits.h"

namespace Gamma {
  class AbstractScene : public Initable, public Destroyable {
  public:
    static AbstractScene* active;

    virtual ~AbstractScene() {};
  };
}