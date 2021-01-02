#pragma once

#include "math/matrix.h"
#include "math/orientation.h"
#include "math/vector.h"

namespace Gamma {
  struct Camera {
    Vec3f position;
    Orientation orientation;

    static Camera* active;
  };
}