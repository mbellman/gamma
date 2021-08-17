#pragma once

#include "system/type_aliases.h"

#if __has_include("gamma_flags.h")
  // Include custom #define flag overrides
  #include "gamma_flags.h"
#endif

namespace Gamma {
  enum GammaFlags {
    FREE_CAMERA_MODE = 1 << 0,
    MOVABLE_OBJECTS = 1 << 1,  // @todo
    WIREFRAME_MODE = 1 << 2,
    VSYNC = 1 << 3,
    RENDER_REFLECTIONS = 1 << 4,
    RENDER_REFRACTIONS = 1 << 5,
    RENDER_SHADOWS = 1 << 6
  };

  void Gm_DisableFlags(GammaFlags flags);
  void Gm_EnableFlags(GammaFlags flags);
  uint32 Gm_GetFlags();
}