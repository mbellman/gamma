#pragma once

#include "math/geometry.h"
#include "math/matrix.h"
#include "math/vector.h"

namespace Gamma {
  enum ObjectFlags {
    IS_DIRTY
  };

  struct Object {
    int flags = 0;
    Vec3f _position;
    Vec3f _rotation;
    Vec3f _scale;
    Matrix4f _matrix;

    inline const Vec3f& position();
    inline void position(const Vec3f& position);
    inline const Vec3f& rotation();
    inline void rotation(const Vec3f& rotation);
    inline const Vec3f& scale();
    inline void scale(const Vec3f& scale);
  };

  struct Mesh {
    Polygon* polygons = nullptr;
  };

  Mesh* Gm_LoadMesh(const char* path);
  void Gm_FreeMesh(Mesh* mesh);
  void Gm_RecomputeObjectMatrix(Object* object);
}