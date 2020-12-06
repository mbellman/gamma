#pragma once

#include "Traits.h"

namespace Gamma {
  class AbstractRenderer : public Initable, public Renderable, public Destroyable {
  public:
    virtual ~AbstractRenderer() {};

    virtual void onInit() = 0;
    virtual void onRender() = 0;
    virtual void onDestroy() = 0;
  };
}