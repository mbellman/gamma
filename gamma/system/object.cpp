#include <cstdio>
#include <cstdlib>

#include "system/object.h"

namespace Gamma {
  /**
   * Object
   * ------
   */
  inline const Vec3f& Object::position() {
    return _position;
  }

  inline void Object::position(const Vec3f& position) {
    _position = position;
    flags |= ObjectFlags::IS_DIRTY;
  }

  inline const Vec3f& Object::rotation() {
    return _rotation;
  }

  inline void Object::rotation(const Vec3f& rotation) {
    _rotation = rotation;
    flags |= ObjectFlags::IS_DIRTY;
  }

  inline const Vec3f& Object::scale() {
    return _scale;
  }

  inline void Object::scale(const Vec3f& scale) {
    _scale = scale;
    flags |= ObjectFlags::IS_DIRTY;
  }

  /**
   * Gm_LoadMesh
   * -----------
   */
  Mesh* Gm_LoadMesh(const char* path) {
    // @TODO malloc()?
    Mesh* mesh = new Mesh();

    return mesh;
  }

  /**
   * Gm_FreeMesh
   * -----------
   */
  void Gm_FreeMesh(Mesh* mesh) {
    // @TODO free()?
    delete mesh;
  }

  /**
   * Gm_RecomputeObjectMatrix
   * ------------------------
   */
  void Gm_RecomputeObjectMatrix(Object* object) {
    // @TODO
  }
}