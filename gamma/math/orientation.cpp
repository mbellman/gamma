#include <cmath>

#include "math/constants.h"
#include "math/orientation.h"
#include "math/vector.h"

namespace Gamma {
  Vec3f Orientation::getDirection() const {
    return Vec3f(
      sinf(yaw) * cosf(pitch),
      -sinf(pitch),
      cosf(yaw) * cosf(pitch)
    ).unit();
  }

  Vec3f Orientation::getLeftDirection() const {
    return Vec3f::cross(getDirection(), getUpDirection()).unit();
  }

  Vec3f Orientation::getRightDirection() const {
    return Vec3f::cross(getUpDirection(), getDirection()).unit();
  }

  Vec3f Orientation::getUpDirection() const {
    return Orientation(roll, pitch - PI / 2.0f, yaw).getDirection();
  }

  void Orientation::face(const Vec3f& forward, const Vec3f& up) {
    // @todo
  }

  Vec3f Orientation::toVec3f() const {
    return Vec3f(pitch, yaw, roll);
  }
}