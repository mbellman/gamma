#pragma once

#include "math/matrix.h"
#include "math/vector.h"

namespace Gamma {
  struct Orientation {
    float roll;
    float pitch;
    float yaw;

    Vec3f toVec3f() {
      return Vec3f(pitch, yaw, roll);
    }
  };

  struct Camera {
    Vec3f position;
    Orientation orientation;

    static Camera* active;

    Matrix4f createViewMatrix() const;
  };
}