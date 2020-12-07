#pragma once

namespace Gamma {
  struct Vec2f {
    Vec2f() {};
    Vec2f(float x, float y) : x(x), y(y) {};

    float x = 0.0f;
    float y = 0.0f;

    const float* float2() const;
  };

  struct Vec3f : Vec2f {
    Vec3f() {};
    Vec3f(float f) : Vec2f(f, f), z(f) {};
    Vec3f(float x, float y, float z) : Vec2f(x, y), z(z) {};

    float z = 0.0f;

    static Vec3f crossProduct(const Vec3f& v1, const Vec3f& v2);

    Vec3f operator+(const Vec3f& vector) const;
    void operator+=(const Vec3f& vector);
    Vec3f operator-(const Vec3f& vector) const;
    void operator-=(const Vec3f& vector);
    Vec3f operator*(float scalar) const;
    Vec3f operator*(const Vec3f& vector) const;
    void operator*=(float scalar);
    Vec3f operator/(float scalar) const;

    const float* float3() const;
    Vec3f gl() const;
    Vec3f invert() const;
    float magnitude() const;
    Vec3f unit() const;
    Vec3f xz() const;
  };
}