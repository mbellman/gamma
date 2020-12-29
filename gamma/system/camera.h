#pragma once

#include "math/matrix.h"
#include "math/vector.h"

namespace Gamma {
  struct Orientation {
    float roll = 0.0f;
    float pitch = 0.0f;
    float yaw = 0.0f;

    Vec3f toVec3f() {
      return Vec3f(pitch, yaw, roll);
    }

    // @TODO getDirection(), getLeftDirection(), getRightDirection(), face(const Vec3f& direction), etc.
  };

  struct Camera {
    Vec3f position;
    Orientation orientation;

    static Camera* active;

    Matrix4f createViewMatrix() const;
  };
}