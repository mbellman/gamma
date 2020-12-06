#pragma once

#include "AbstractRenderer.h"

namespace Gamma {
  class OpenGLRenderer final : public AbstractRenderer {
  public:
    ~OpenGLRenderer();

    virtual void onInit() override;
    virtual void onRender() override;
    virtual void onDestroy() override;
  };
}