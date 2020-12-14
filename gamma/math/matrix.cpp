#include <cmath>

#include "math/matrix.h"
#include "math/Quaternion.h"

namespace Gamma {
  /**
   * Matrix4f
   * -------
   */
  Matrix4f Matrix4f::identity() {
    return {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
    };
  }

  Matrix4f Matrix4f::lookAt(const Vec3f& eye, const Vec3f& direction, const Vec3f& top) {
    Vec3f forward = direction.unit();
    Vec3f right = Vec3f::crossProduct(top, forward).unit();
    Vec3f up = Vec3f::crossProduct(forward, right).unit();
    Matrix4f translation = Matrix4f::translate(eye.invert());

    Matrix4f rotation = {
      right.x, right.y, right.z, 0.0f,
      up.x, up.y, up.z, 0.0f,
      forward.x, forward.y, forward.z, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
    };

    return rotation * translation;
  }

  Matrix4f Matrix4f::orthographic(float top, float bottom, float left, float right, float near, float far) {
    return {
      2.0f / (right - left), 0.0f, 0.0f, -(right + left) / (right - left),
      0.0f, 2.0f / (top - bottom), 0.0f, -(top + bottom) / (top - bottom),
      0.0f, 0.0f, -2.0f / (far - near), -(far + near) / (far - near),
      0.0f, 0.0f, 0.0f, 1.0f
    };
  }

  Matrix4f Matrix4f::projection(const Area<unsigned int>& area, float fov, float near, float far) {
    constexpr float PI = 3.141592f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    float f = 1.0f / tanf(fov / 2.0f * DEG_TO_RAD);
    float aspectRatio = (float)area.width / (float)area.height;

    return {
      f / aspectRatio, 0.0f, 0.0f, 0.0f,
      0.0f, f, 0.0f, 0.0f,
      0.0f, 0.0f, (far + near) / (near - far), (2 * far * near) / (near - far),
      0.0f, 0.0f, -1.0f, 0.0f
    };
  }

  Matrix4f Matrix4f::rotate(const Vec3f& rotation) {
    Quaternion pitch = Quaternion::fromAxisAngle(rotation.x, 1.0f, 0.0f, 0.0f);
    Quaternion yaw = Quaternion::fromAxisAngle(rotation.y, 0.0f, 1.0f, 0.0f);
    Quaternion roll = Quaternion::fromAxisAngle(rotation.z, 0.0f, 0.0f, 1.0f);

    return (pitch * yaw * roll).toMatrix4f();
  }

  Matrix4f Matrix4f::scale(const Vec3f& scale) {
    return {
      scale.x, 0.0f, 0.0f, 0.0f,
      0.0f, scale.y, 0.0f, 0.0f,
      0.0f, 0.0f, scale.z, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
    };
  }

  Matrix4f Matrix4f::translate(const Vec3f& translation) {
    return {
      1.0f, 0.0f, 0.0f, translation.x,
      0.0f, 1.0f, 0.0f, translation.y,
      0.0f, 0.0f, 1.0f, translation.z,
      0.0f, 0.0f, 0.0f, 1.0f
    };
  }

  Matrix4f Matrix4f::transpose() const {
    return {
      m[0], m[4], m[8], m[12],
      m[1], m[5], m[9], m[13],
      m[2], m[6], m[10], m[14],
      m[3], m[7], m[11], m[15]
    };
  }

  Matrix4f Matrix4f::operator*(const Matrix4f& matrix) const {
    Matrix4f product;

    for (int r = 0; r < 4; r++) {
      for (int c = 0; c < 4; c++) {
        float& value = product.m[r * 4 + c] = 0;

        for (int n = 0; n < 4; n++) {
          value += m[r * 4 + n] * matrix.m[n * 4 + c];
        }
      }
    }

    return product;
  }

  Vec3f Matrix4f::operator*(const Vec3f& vector) const {
    float x = vector.x;
    float y = vector.y;
    float z = vector.z;
    float w = 1.0f;

    return Vec3f(
      x * m[0] + y * m[1] + z * m[2] + w * m[3],
      x * m[4] + y * m[5] + z * m[6] + w * m[7],
      x * m[8] + y * m[9] + z * m[10] + w * m[11]
    );
  }
}