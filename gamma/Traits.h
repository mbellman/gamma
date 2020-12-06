#pragma once

namespace Gamma {
  struct Initable {
    virtual void onInit() = 0;
  };

  struct Destroyable {
    virtual void onDestroy() = 0;
  };

  struct Renderable {
    virtual void onRender() = 0;
  };
}