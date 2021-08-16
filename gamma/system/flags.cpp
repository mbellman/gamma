#include "system/flags.h"
#include "system/type_aliases.h"

namespace Gamma {
  static uint32 internalFlags = GammaFlags::RENDER_REFLECTIONS | GammaFlags::RENDER_REFRACTIONS;

  void Gm_DisableFlags(GammaFlags flags) {
    internalFlags &= ~flags;
  }

  void Gm_EnableFlags(GammaFlags flags) {
    internalFlags |= flags;    
  }

  uint32 Gm_GetFlags() {
    return internalFlags;
  }
}