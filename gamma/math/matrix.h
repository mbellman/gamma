#pragma once

#include "math/plane.h"
#include "math/vector.h"
#include "system/type_aliases.h"

namespace Gamma {
  struct Matrix4f {
    float m[16];

    static Matrix4f identity();
    static Matrix4f lookAt(const Vec3f& eye, const Vec3f& direction, const Vec3f& top);
    static Matrix4f orthographic(float top, float bottom, float left, float right, float near, float far);
    static Matrix4f projection(const Area<uint32>& area, float fov, float near, float far);
    static Matrix4f rotate(const Vec3f& rotation);
    static Matrix4f scale(const Vec3f& scale);
    static Matrix4f translate(const Vec3f& translation);

    Matrix4f operator*(const Matrix4f& matrix) const;
    Vec3f operator*(const Vec3f& vector) const;

    void debug() const;
    Matrix4f transpose() const;
  };
}