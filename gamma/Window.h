#pragma once

#include "AbstractRenderer.h"
#include "Geometry.h"
#include "Traits.h"

namespace Gamma {
  class Window final {
  public:
    ~Window();

    void open();
    void setRenderer(AbstractRenderer* renderer);
    void setRegion(const Region<unsigned int>& region);

  private:
    AbstractRenderer* renderer = nullptr;
    Region<unsigned int> region = { 0, 0, 640, 480 };
  };
}