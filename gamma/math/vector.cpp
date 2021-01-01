#include <math.h>

#include "math/Vector.h"

namespace Gamma {
  /**
   * Vec2f
   * -----
   */
  const float* Vec2f::float2() const {
    return &x;
  }

  /**
   * Vec3f
   * -----
   */
  Vec3f Vec3f::operator+(const Vec3f& vector) const {
    return {
      x + vector.x,
      y + vector.y,
      z + vector.z
    };
  }

  void Vec3f::operator+=(const Vec3f& vector) {
    x += vector.x;
    y += vector.y;
    z += vector.z;
  }

  Vec3f Vec3f::operator-(const Vec3f& vector) const {
    return {
      x - vector.x,
      y - vector.y,
      z - vector.z
    };
  }

  void Vec3f::operator-=(const Vec3f& vector) {
    x -= vector.x;
    y -= vector.y;
    z -= vector.z;
  }

  Vec3f Vec3f::operator*(float scalar) const {
    return {
      x * scalar,
      y * scalar,
      z * scalar
    };
  }

  Vec3f Vec3f::operator*(const Vec3f& vector) const {
    return {
      x * vector.x,
      y * vector.y,
      z * vector.z
    };
  }

  void Vec3f::operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;
  }

  Vec3f Vec3f::operator/(float scalar) const {
    return {
      x / scalar,
      y / scalar,
      z / scalar
    };
  }

  Vec3f Vec3f::cross(const Vec3f& v1, const Vec3f& v2) {
    return {
      v1.y * v2.z - v1.z * v2.y,
      v1.z * v2.x - v1.x * v2.z,
      v1.x * v2.y - v1.y * v2.x
    };
  }

  const float* Vec3f::float3() const {
    return &x;
  }

  Vec3f Vec3f::gl() const {
    return *this * Vec3f(1.0f, 1.0f, -1.0f);
  }

  Vec3f Vec3f::invert() const {
    return *this * -1.0f;
  }

  float Vec3f::magnitude() const {
    return sqrtf(x*x + y*y + z*z);
  }

  Vec3f Vec3f::unit() const {
    float m = magnitude();

    return {
      x / m,
      y / m,
      z / m
    };
  }

  Vec3f Vec3f::xz() const {
    return *this * Vec3f(1.0f, 0.0f, 1.0f);
  }
}