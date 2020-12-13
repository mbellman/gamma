#pragma once

#include <vector>

#include "math/geometry.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "system/type_aliases.h"

namespace Gamma {
  enum ObjectFlags {
    IS_DIRTY = 1 << 0
  };

  enum LightType {
    POINT,
    DIRECTIONAL,
    SPOT
  };

  /**
   * Mesh
   * ----
   *
   * A Mesh serves as a static reference model from which
   * individual Objects can be created, where Objects
   * only contain transformation properties and other
   * instance-specific attributes.
   */
  struct Mesh {
    /**
     * Static mesh vertices in model space.
     */
    std::vector<Vertex> vertices;
    /**
     * Vertex indices for each triangle face of the mesh,
     * defined in groups of three.
     */
    std::vector<uint32> faceIndexes;
    /**
     * Transformation matrices for each object created from
     * the mesh. Variable in size as descendant objects are
     * created or destroyed.
     *
     * @TODO see if a vector is appropriate here for object
     * collections with members being repeatedly inserted,
     * removed, or conditionally rendered
     */
    std::vector<Matrix4f> matrices;
  };

  /**
   * BaseEntity
   * ----------
   *
   * Defines base attributes extensible to any dynamic
   * elements in a scene, such as Objects or Lights.
   */
  struct BaseEntity {
    /**
     * Controls how long an entity will remain in the scene
     * before being subject to removal, in milliseconds.
     * Entities with lifetimes of -1 are ignored and allowed
     * to remain in the scene indefinitely.
     */
    int lifetime = -1;
  };

  /**
   * Object
   * ------
   *
   * Objects are derived from Meshes, defining individual
   * instances of a Mesh distributed throughout a scene,
   * each with its own transformations and properties.
   */
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

  /**
   * Light
   * -----
   *
   * Defines a light source, which affects scene illumination
   * and color/reflective properties of illuminated surfaces.
   */
  struct Light : BaseEntity {
    Vec3f position;
    Vec3f color = Vec3f(1.0f);
    float radius = 100.0f;
    float power = 1.0f;
    LightType type = LightType::POINT;
    bool canCastShadows = false;
  };

  /**
   * Gm_CreateCube
   * -------------
   */
  Mesh* Gm_CreateCube();

  /**
   * Gm_FreeMesh
   * -----------
   */
  void Gm_FreeMesh(Mesh* mesh);

  /**
   * Gm_LoadMesh
   * -----------
   */
  Mesh* Gm_LoadMesh(const char* path);

  /**
   * Gm_RecomputeObjectMatrix
   * ------------------------
   */
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