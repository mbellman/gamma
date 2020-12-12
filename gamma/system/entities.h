#pragma once

#include "math/geometry.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "system/type_aliases.h"

namespace Gamma {
  enum ObjectFlags {
    IS_DIRTY = 1 << 0
  };

  enum Primitive {
    CUBE,
    ICOSPHERE
  };

  enum LightType {
    POINT,
    DIRECTIONAL,
    SPOT
  };

  struct Mesh {
    Polygon* polygons = nullptr;
    float* matrices = nullptr;
  };

  struct BaseEntity {
    int lifetime = -1;
  };

  struct Object : BaseEntity {
    uint8 _flags = 0;
    Vec3f _position;
    Vec3f _rotation;
    Vec3f _scale;

    const Vec3f& position() const;
    void position(const Vec3f& position);
    void remove();
    const Vec3f& rotation() const;
    void rotation(const Vec3f& rotation);
    const Vec3f& scale() const;
    void scale(const Vec3f& scale);
    void scale(float scale);
  };

  struct Light : BaseEntity {
    Vec3f position;
    Vec3f color = Vec3f(1.0f);
    float radius = 100.0f;
    float power = 1.0f;
    LightType type = LightType::POINT;
    bool canCastShadows = false;
  };

  Mesh* Gm_CreatePrimitiveMesh(Primitive primitive);
  void Gm_FreeMesh(Mesh* mesh);
  Mesh* Gm_LoadMesh(const char* path);
  void Gm_RecomputeObjectMatrix(Object* object);

  /**
   * Object
   * ------
   */
  inline const Vec3f& Object::position() const {
    return _position;
  }

  inline void Object::position(const Vec3f& position) {
    _position = position;
    _flags |= ObjectFlags::IS_DIRTY;
  }

  inline void Object::remove() {
    lifetime = 0;
  }

  inline const Vec3f& Object::rotation() const {
    return _rotation;
  }

  inline void Object::rotation(const Vec3f& rotation) {
    _rotation = rotation;
    _flags |= ObjectFlags::IS_DIRTY;
  }

  inline const Vec3f& Object::scale() const {
    return _scale;
  }

  inline void Object::scale(const Vec3f& scale) {
    _scale = scale;
    _flags |= ObjectFlags::IS_DIRTY;
  }

  inline void Object::scale(float scale) {
    this->scale(Vec3f(scale));
  }
}