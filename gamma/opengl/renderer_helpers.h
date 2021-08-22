#pragma once

#include "opengl/OpenGLRenderer.h"

namespace Gamma {
  void Gm_InitDeferredRenderPath(DeferredPath& deferred, const Area<uint32>& internalResolution);
  void Gm_DestroyDeferredRenderPath(DeferredPath& deferred);
}