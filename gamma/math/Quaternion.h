#pragma once

namespace Gamma {
  struct Matrix4f;

  /**
   * Quaternion
   * ----------
   *
   * @TODO description
   */
  struct Quaternion {
    float w;
    float x;
    float y;
    float z;

    static Quaternion fromAxisAngle(float angle, float x, float y, float z);

    Matrix4f toMatrix4f() const;
    Quaternion operator*(const Quaternion& q2) const;
  };
}