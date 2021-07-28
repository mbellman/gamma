#pragma once

#include <vector>

#include "math/geometry.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "system/type_aliases.h"

namespace Gamma {
  enum LightType {
    POINT,
    DIRECTIONAL,
    SPOT
  };

  /**
   * ObjectRecord
   * ------------
   *
   * A unique identifier for an object which allows it
   * to be looked up on the appropriate Mesh and at its
   * corresponding index, with ID checks for referential
   * integrity.
   */
  struct ObjectRecord {
    uint16 meshIndex;
    uint16 meshId;
    uint16 index;
    uint16 id;
  };

  /**
   * Object
   * ------
   *
   * Objects are derived from Meshes, defining individual
   * instances of a Mesh distributed throughout a scene,
   * each with its own transformations.
   */
  struct Object {
    ObjectRecord _record;
    Vec3f position;
    Vec3f rotation;
    Vec3f scale;
  };

  /**
   * ObjectPool
   * ----------
   *
   * @todo description
   */
  class ObjectPool {
  public:
    Object& operator [](uint16 index);

    Object* begin() const;
    Object& createObject();
    Object* end() const;
    void free();
    Matrix4f* getMatrices() const;
    uint16 max() const;
    void remove(uint16 index);
    void reserve(uint16 size);
    uint16 total() const;
    void transform(uint16 index, const Matrix4f& matrix);

  private:
    Object* objects = nullptr;
    Matrix4f* matrices = nullptr;
    uint16 maxObjects = 0;
    uint16 totalActiveObjects = 0;
    uint16 runningId = 0;
  };

  /**
   * Light
   * -----
   *
   * Defines a light source, which affects scene illumination
   * and color/reflective properties of illuminated surfaces.
   */
  struct Light {
    Vec3f position;
    float radius = 100.0f;
    Vec3f color = Vec3f(1.0f);
    float power = 1.0f;
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
     * The index of the mesh in a scene's Mesh array,
     * used for efficient mesh lookups.
     */
    uint16 index = 0;
    /**
     * A unique ID for the mesh. If a mesh retrieved
     * at a specific index in a scene's Mesh array
     * does not match an expected ID (e.g., if the
     * mesh structure has been recycled), the reference
     * should be considered stale.
     */
    uint16 id = 0;
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
     * A collection of objects representing unique instances
     * of the mesh.
     */
    ObjectPool objects;
  };

  /**
   * Gm_ComputeNormals
   * -----------------
   */
  void Gm_ComputeNormals(Mesh* mesh);

  /**
   * Gm_ComputeTangents
   * ------------------
   */
  void Gm_ComputeTangents(Mesh* mesh);

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
}