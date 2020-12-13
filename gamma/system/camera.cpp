#include "system/camera.h"

namespace Gamma {
  Camera* Camera::active = nullptr;

  Matrix4f Camera::createViewMatrix() const {
    // @TODO quaternion multiplication + matrix conversion
    return Matrix4f();
  }
}