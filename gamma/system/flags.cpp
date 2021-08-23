#include "system/flags.h"
#include "system/type_aliases.h"

namespace Gamma {
  static uint32 internalFlags =
    GammaFlags::RENDER_REFLECTIONS |
    GammaFlags::RENDER_REFRACTIVE_OBJECTS |
    GammaFlags::RENDER_SHADOWS |
    GammaFlags::SHOW_DEV_BUFFERS;

  void Gm_DisableFlags(GammaFlags flags) {
    internalFlags &= ~flags;
  }

  void Gm_EnableFlags(GammaFlags flags) {
    internalFlags |= flags;    
  }

  uint32 Gm_GetFlags() {
    return internalFlags;
  }

  bool Gm_IsFlagEnabled(GammaFlags flag) {
    return internalFlags & flag;
  }
}